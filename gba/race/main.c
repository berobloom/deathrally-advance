// Death Rally on the GBA: a race run by the game's own race code.
//
// race_logic() and race_render() (built with DR_RACE_NODRAW, so it keeps
// the camera, animations and lap times without drawing) run as on the
// desktop. The GBA draws the result its own way, as gba/proto does: the
// track as a tile background streamed from ROM, the cars as 64x64 sprites
// with their shadows through the OBJ window. The race starts from the start
// of one the desktop game recorded (make ticklog), baked by tools/gbabake.
//
// D-pad left/right steer, A accelerates, B brakes, R turbo, L machine gun,
// Select drops a mine. Start lets the AI drive your car, or starts the race
// again once it's over.

#include <gba_base.h>
#include <gba_video.h>
#include <gba_sprites.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_dma.h>
#include <gba_timers.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "drally.h"
#include "drally_keyboard.h"
#include "drally_structs_free.h"
#include "drally_race.h"
#include "race_replay.h"

// baked by tools/gbabake, see assets.S
extern const u16 track_pal[256];
extern const u8 track_tiles[];
extern const u32 track_inf[];
extern const u8 cars_obj[];
extern const __DWORD__ race_bin[];

void race_logic(void);
void race___40164h(void);

	extern __BYTE__ ___1e6ed0h[];
	extern __BYTE__ ___243ce0h[];
	extern __BYTE__ ___2438c8h[];
	extern __DWORD__ MY_CAR_IDX;
	extern int NUM_OF_CARS;
	extern int X___243c8ch;
	extern int Y___243c90h;
	extern int CURRENT_VIEWPORT_W;
	extern int CURRENT_VIEWPORT_X;
	extern int CURRENT_VIEWPORT_CENTER_X;
	extern kb_control_t ___1a1140h;

#define REG_WAITCNT		(*(vu16 *)0x04000204)
// WININ/WINOUT layer bits and BLDCNT fields, which libgba leaves out
#define WIN_BG0			(1 << 0)
#define WIN_BG1			(1 << 1)
#define WIN_OBJ			(1 << 4)
#define WIN_BLD			(1 << 5)
#define BLEND_BG0		(1 << 0)
#define BLEND_DARKEN		(3 << 6)
#define REG_DEBUG_ENABLE	(*(vu16 *)0x04FFF780)
#define REG_DEBUG_FLAGS		(*(vu16 *)0x04FFF700)
#define REG_DEBUG_STRING	((char *)0x04FFF600)

#define SCREEN_W		240
#define SCREEN_H		160
#define CAR_FRAMES		96
#define CAR_IMG_BYTES		(40*40)		// a rotation frame in the game's car images
#define CAR_OBJ_BYTES		(64*64)
#define CAR_PAD			4		// the 40x40 frame sits at (4,4) in its 64x64 sprite
#define CAR_UPLOAD_BYTES	(6*8*64)	// so only the first 6 rows of tiles hold pixels
#define NUM_CARS		4
// the game's timer runs at 70 Hz. A tick of the logic takes up to about
// 210,000 cycles (gba/replay), so 70 of them leave too little of the
// GBA's 60 frames for the rest: by default the race runs a tick a frame,
// at 6/7 of its speed. make TICKS=70 gives the real speed and drops frames
#if !defined(TICKS_PER_SEC)
#define TICKS_PER_SEC		60
#endif
#define VBLANKS_PER_SEC		60
#define FRAME_CYCLES		280896		// one 59.73 Hz frame of the 16.78 MHz CPU
// at most what one frame is due, so that a slow frame isn't made up for
// with more ticks, which would make the next one slower still
#define MAX_TICKS		((TICKS_PER_SEC+VBLANKS_PER_SEC-1)/VBLANKS_PER_SEC)
// unmapped on the GBA: writes go nowhere, also on real hardware, where a
// write to ROM could reach a flash cartridge's command registers
#define NOWHERE			((__POINTER__)0x00010000)
#define RACE_OVER		0x12c		// ___2438c8h counts up past this once the race is over

// the track background: a ring of 32x22 tiles in charblocks 0-2, map in screenblock 31
#define RING_W			32
#define RING_H			22
#define TRACK_MAP		((u16 *)SCREEN_BASE_BLOCK(31))
#define TRACK_CHARS		((u8 *)CHAR_BASE_BLOCK(0))
// the HUD: 8bpp glyphs in charblock 3, map in screenblock 30
#define HUD_MAP			((u16 *)SCREEN_BASE_BLOCK(30))
#define HUD_CHARS		((u8 *)CHAR_BASE_BLOCK(3))

// the keys race___40164h() reads, set in ___1a1140h
#define KEY_SCAN_ACCELERATE	DR_SCAN_A
#define KEY_SCAN_BRAKE		DR_SCAN_Z
#define KEY_SCAN_TURBO		DR_SCAN_S
#define KEY_SCAN_GUN		DR_SCAN_X
#define KEY_SCAN_MINE		DR_SCAN_SPACE
#define KEY_SCAN_HORN		DR_SCAN_H

__BYTE__ kmap[256];

// the game's timer interrupt counts these; nothing here waits on them
__DWORD__ __GET_FRAME_COUNTER(void){ return 0; }

// no sound yet
void dRally_Sound_pushEffect(__BYTE__ channel, __BYTE__ n, __DWORD__ unk, __DWORD__ a0, __DWORD__ a1, __DWORD__ a2){}
void dRally_Sound_freeEffectChannel(__BYTE__ ch_num){}
void dRally_Sound_adjustEffect(__BYTE__ channel, __DWORD__ vol, __DWORD__ freq, __DWORD__ balance){}

EWRAM_BSS static __BYTE__ flat[0x4000] __attribute__((aligned(4)));
EWRAM_BSS static __BYTE__ obstacles[0x800];

static int track_w, track_h;		// in tiles
static int track_pw, track_ph;		// in pixels; the tiles are padded to a multiple of 8
static int cam_x, cam_y;
static int vis_x0 = -1, vis_y0, vis_x1, vis_y1;
static int shown_frame[NUM_CARS];	// rotation frame in each car's OBJ VRAM
static volatile u32 vblanks;
static u8 ring_row[256];		// ty % RING_H, without dividing
static int debug_log;

static void copy32(void * dst, const void * src, u32 bytes){

	REG_DMA3SAD = (u32)src;
	REG_DMA3DAD = (u32)dst;
	REG_DMA3CNT = DMA_ENABLE | DMA32 | (bytes >> 2);
}

// to mGBA's debug log
static void say(const char * fmt, ...){

	va_list 	ap;

	if(!debug_log) return;
	va_start(ap, fmt);
	vsnprintf(REG_DEBUG_STRING, 256, fmt, ap);
	va_end(ap);
	REG_DEBUG_FLAGS = 3 | 0x100;	// info level, print
}

// timers 2 and 3 cascaded: CPU cycles
static u32 cycles(void){

	u16 	hi, lo;

	do {
		hi = REG_TM3CNT_L;
		lo = REG_TM2CNT_L;
	} while(hi != REG_TM3CNT_L);

	return ((u32)hi << 16) | lo;
}

static void vblank_handler(void){

	vblanks++;
}

// --- track background ----------------------------------------------------

static void load_tile(int tx, int ty){

	int slot = (tx & (RING_W-1)) + ring_row[ty]*RING_W;

	copy32(TRACK_CHARS + 64*slot, track_tiles + 64*(ty*track_w + tx), 64);
	TRACK_MAP[(ty & 31)*32 + (tx & 31)] = slot;
}

// load the tiles that came into view since the last call, then scroll
static void update_track(void){

	int x0 = cam_x >> 3, y0 = cam_y >> 3;
	int x1 = (cam_x + SCREEN_W - 1) >> 3, y1 = (cam_y + SCREEN_H - 1) >> 3;
	int tx, ty;

	if((vis_x0 < 0)||(x0 > vis_x1)||(x1 < vis_x0)||(y0 > vis_y1)||(y1 < vis_y0)){

		for(ty = y0; ty <= y1; ty++) for(tx = x0; tx <= x1; tx++) load_tile(tx, ty);
	}
	else {

		// only the columns, then the rows, that weren't on screen before
		for(tx = x0; tx < vis_x0; tx++) for(ty = y0; ty <= y1; ty++) load_tile(tx, ty);
		for(tx = vis_x1+1; tx <= x1; tx++) for(ty = y0; ty <= y1; ty++) load_tile(tx, ty);
		for(ty = y0; ty < vis_y0; ty++) for(tx = x0; tx <= x1; tx++) if((tx >= vis_x0)&&(tx <= vis_x1)) load_tile(tx, ty);
		for(ty = vis_y1+1; ty <= y1; ty++) for(tx = x0; tx <= x1; tx++) if((tx >= vis_x0)&&(tx <= vis_x1)) load_tile(tx, ty);
	}

	vis_x0 = x0; vis_y0 = y0; vis_x1 = x1; vis_y1 = y1;
	REG_BG0HOFS = cam_x;
	REG_BG0VOFS = cam_y;
}

// --- HUD -------------------------------------------------------------------

// 5x7 glyphs, one byte per row, bit 4 is the leftmost pixel
static const char hud_chars[] = "0123456789FPSCU%GXLAREOV/ ";
static const u8 hud_font[][7] = {
	{14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31}, {31,2,4,2,1,17,14},
	{2,6,10,18,31,2,2}, {31,16,30,1,1,17,14}, {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8},
	{14,17,17,14,17,17,14}, {14,17,17,15,1,2,12}, {31,16,16,30,16,16,16}, {30,17,17,30,16,16,16},
	{15,16,16,14,1,1,30}, {14,17,16,16,16,17,14}, {17,17,17,17,17,17,14}, {24,25,2,4,8,19,3},
	{14,17,16,23,17,17,15}, {17,17,10,4,10,17,17}, {16,16,16,16,16,16,31}, {14,17,17,31,17,17,17},
	{30,17,17,30,20,18,17}, {31,16,16,30,16,16,31}, {14,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
	{1,1,2,4,8,16,16}, {0,0,0,0,0,0,0}
};

static u8 hud_glyph[128];		// glyph number of each character

static int closest_color(int r, int g, int b){

	int n, best = 1, best_d = 0x7fffffff;

	for(n = 1; n < 256; n++){

		int c = track_pal[n], dr = (c & 31) - r, dg = ((c >> 5) & 31) - g, db = ((c >> 10) & 31) - b;
		int d = dr*dr + dg*dg + db*db;
		if(d < best_d){ best_d = d; best = n; }
	}

	return best;
}

// glyphs in white with a black outline, so they read on any part of the track
static void init_hud(void){

	int white = closest_color(31, 31, 31), black = closest_color(0, 0, 0);
	int g, x, y;

	for(g = 0; g < (int)sizeof(hud_chars)-1; g++){

		u8 px[8][8];
		memset(px, 0, sizeof(px));

		for(y = 0; y < 7; y++) for(x = 0; x < 5; x++) if(hud_font[g][y] & (16 >> x)){

			int dx, dy;
			for(dy = -1; dy <= 1; dy++) for(dx = -1; dx <= 1; dx++){

				int ox = x+1+dx, oy = y+dy;
				if((ox >= 0)&&(ox < 8)&&(oy >= 0)&&(oy < 8)&&(px[oy][ox] == 0)) px[oy][ox] = black;
			}
		}
		for(y = 0; y < 7; y++) for(x = 0; x < 5; x++) if(hud_font[g][y] & (16 >> x)) px[y][x+1] = white;

		// VRAM takes 16-bit writes only
		for(y = 0; y < 8; y++) for(x = 0; x < 8; x += 2)
			((u16 *)(HUD_CHARS + 64*g))[(8*y+x)/2] = px[y][x] | (px[y][x+1] << 8);
	}

	for(g = 0; g < 128; g++) hud_glyph[g] = sizeof(hud_chars)-2;	// space
	for(g = 0; g < (int)sizeof(hud_chars)-1; g++) hud_glyph[(int)hud_chars[g]] = g;
	for(x = 0; x < 32*32; x++) HUD_MAP[x] = sizeof(hud_chars)-2;
}

static void hud_print(int col, int row, const char * s){

	for(; *s; s++, col++) HUD_MAP[row*32 + col] = hud_glyph[*s & 127];
}

static char * put_num(char * p, int v, int digits){

	int n;

	for(n = digits-1; n >= 0; n--, v /= 10) p[n] = (n < digits-1)&&(v == 0) ? ' ' : '0' + v%10;
	return p + digits;
}

// --- cars --------------------------------------------------------------------

static struct_35e_t * car(int n){

	return &((struct_35e_t *)___1e6ed0h)[n];
}

static int car_on_screen(int n){

	int sx = (int)car(n)->XLocation - cam_x, sy = (int)car(n)->YLocation - cam_y;

	return (sx > -32)&&(sx < SCREEN_W+32)&&(sy > -32)&&(sy < SCREEN_H+32);
}

// copy a car's rotation frame into its OBJ VRAM if it changed and is in
// view. cars.obj holds the race's car images in their order, so the frame is
// where ImgOffset points into them
static void upload_car(int n){

	int frame = car(n)->ImgOffset/CAR_IMG_BYTES;

	if((frame == shown_frame[n])||!car_on_screen(n)) return;
	copy32((u8 *)SPRITE_GFX + n*CAR_OBJ_BYTES, cars_obj + frame*CAR_OBJ_BYTES, CAR_UPLOAD_BYTES);
	shown_frame[n] = frame;
}

static void place_cars(void){

	int n;

	for(n = 0; n < NUM_CARS; n++){

		int sx = (int)car(n)->XLocation - cam_x - (CAR_PAD+20), sy = (int)car(n)->YLocation - cam_y - (CAR_PAD+20);
		int tile = n*CAR_OBJ_BYTES/32;
		int hidden = (n >= NUM_OF_CARS)||!car_on_screen(n);

		// the car, then its shadow as OBJ window 5 pixels down and to the right
		OAM[2*n].attr0 = hidden ? OBJ_DISABLE : (OBJ_Y(sy) | OBJ_256_COLOR | OBJ_SQUARE);
		OAM[2*n].attr1 = OBJ_X(sx) | OBJ_SIZE(3);
		OAM[2*n].attr2 = OBJ_CHAR(tile) | OBJ_PRIORITY(1);
		OAM[2*n+1].attr0 = hidden ? OBJ_DISABLE : (OBJ_Y(sy+5) | OBJ_256_COLOR | OBJ_SQUARE | OBJ_MODE(2));
		OAM[2*n+1].attr1 = OBJ_X(sx+5) | OBJ_SIZE(3);
		OAM[2*n+1].attr2 = OBJ_CHAR(tile) | OBJ_PRIORITY(1);
	}
}

// --- the race ------------------------------------------------------------------

// the game's camera looks ahead of your car by up to 48 pixels times its
// speed; on the GBA's smaller screen by that much less
static void follow_camera(void){

	cam_x = (int)car(MY_CAR_IDX)->XLocation + X___243c8ch*SCREEN_W/320 - SCREEN_W/2;
	cam_y = (int)car(MY_CAR_IDX)->YLocation + Y___243c90h*SCREEN_H/200 - SCREEN_H/2;
	if(cam_x > track_pw - SCREEN_W) cam_x = track_pw - SCREEN_W;
	if(cam_y > track_ph - SCREEN_H) cam_y = track_ph - SCREEN_H;
	if(cam_x < 0) cam_x = 0;
	if(cam_y < 0) cam_y = 0;
}

static const char * start_race(void){

	const char * 	err;
	int 		n;

	if(race_replay_size() > sizeof(flat)) return "flat[] too small";
	if((err = race_replay_start((__DWORD__ *)race_bin, flat))) return err;

	// the logic draws skid marks into the track image, which isn't kept
	*race_replay_buf(3) = NOWHERE;
	memcpy(obstacles, *race_replay_buf(5), sizeof(obstacles));
	*race_replay_buf(5) = obstacles;

	D(___243ce0h) = 0;	// you drive, not the AI
	// the wide view, which the HUD part of race_render() assumes
	CURRENT_VIEWPORT_W = 0x140;
	CURRENT_VIEWPORT_X = 0;
	CURRENT_VIEWPORT_CENTER_X = CURRENT_VIEWPORT_W/2;

	memset(kmap, 0, sizeof(kmap));
	___1a1140h.accelerate = KEY_SCAN_ACCELERATE;
	___1a1140h.brake = KEY_SCAN_BRAKE;
	___1a1140h.steer_left = DR_SCAN_LEFT;
	___1a1140h.steer_right = DR_SCAN_RIGHT;
	___1a1140h.turbo_boost = KEY_SCAN_TURBO;
	___1a1140h.machine_gun = KEY_SCAN_GUN;
	___1a1140h.drop_mine = KEY_SCAN_MINE;
	___1a1140h.horn = KEY_SCAN_HORN;

	for(n = 0; n < NUM_CARS; n++) shown_frame[n] = -1;
	vis_x0 = -1;

	return 0;
}

// the GBA's keys as the keyboard the game reads. The mine key is cleared
// by race___40164h() once it's seen, so it's set when pressed only
static void read_keys(u16 keys, u16 down){

	kmap[KEY_SCAN_ACCELERATE] = !!(keys & KEY_A);
	kmap[KEY_SCAN_BRAKE] = !!(keys & KEY_B);
	kmap[DR_SCAN_LEFT] = !!(keys & KEY_LEFT);
	kmap[DR_SCAN_RIGHT] = !!(keys & KEY_RIGHT);
	kmap[KEY_SCAN_TURBO] = !!(keys & KEY_R);
	kmap[KEY_SCAN_GUN] = !!(keys & KEY_L);
	if(down & KEY_SELECT) kmap[KEY_SCAN_MINE] = 1;
}

static void hud_race(void){

	char 	line[16], * p = line;

	*p++ = 'P'; p = put_num(p, car(MY_CAR_IDX)->Position, 1);
	*p++ = ' '; *p++ = 'L'; p = put_num(p, car(MY_CAR_IDX)->Lap, 2);
	*p = 0;
	hud_print(0, 19, line);
	hud_print(10, 9, ((int)D(___2438c8h) > 0) ? "RACE OVER" : "         ");
}

int main(void){

	const char * 	err;
	int 		n, frames = 0, over = 0;
	u32 		last_vblank, last_report = 0, ticks_due = 0, t, frame_start;
	u32 		busy = 0, logic = 0, render = 0, ticks = 0, logic_max = 0;
	char 		line[32];

	REG_WAITCNT = 0x4007;		// ROM at 3/2 wait states with prefetch, works on flash carts
	REG_DEBUG_ENABLE = 0xC0DE;
	debug_log = (REG_DEBUG_ENABLE == 0x1DEA);

	irqInit();
	irqSet(IRQ_VBLANK, vblank_handler);
	irqEnable(IRQ_VBLANK);
	REG_TM3CNT_H = TIMER_START | TIMER_COUNT;
	REG_TM2CNT_H = TIMER_START;

	for(n = 0; n < 256; n++) ring_row[n] = n % RING_H;
	track_w = track_inf[0];
	track_h = track_inf[1];
	track_pw = track_inf[2];
	track_ph = track_inf[3];

	REG_DISPCNT = LCDC_OFF;
	copy32(BG_PALETTE, track_pal, 512);
	copy32(SPRITE_PALETTE, track_pal, 512);
	for(n = 0; n < 128; n++) OAM[n].attr0 = OBJ_DISABLE;
	init_hud();

	REG_BG0CNT = BG_256_COLOR | CHAR_BASE(0) | SCREEN_BASE(31) | BG_SIZE_0 | BG_PRIORITY(2);
	REG_BG1CNT = BG_256_COLOR | CHAR_BASE(3) | SCREEN_BASE(30) | BG_SIZE_0 | BG_PRIORITY(0);
	// shadows: inside the OBJ window the track (BG0) is darkened
	REG_WINOUT = (WIN_BG0 | WIN_BG1 | WIN_OBJ) | ((WIN_BG0 | WIN_BG1 | WIN_OBJ | WIN_BLD) << 8);
	REG_BLDCNT = BLEND_BG0 | BLEND_DARKEN;
	REG_BLDY = 9;

	t = vblanks;
	if((err = start_race())){

		hud_print(0, 0, "ERR");
		say("race.bin: %s", err);
		REG_DISPCNT = MODE_0 | BG1_ON;
		while(1) VBlankIntrWait();
	}
	say("race set up in %lu frames", vblanks-t);

	race_render();
	follow_camera();
	update_track();
	for(n = 0; n < NUM_CARS; n++) upload_car(n);
	place_cars();
	VBlankIntrWait();
	REG_DISPCNT = MODE_0 | BG0_ON | BG1_ON | OBJ_ON | OBJ_1D_MAP | OBJ_WIN_ON;
	last_vblank = vblanks;

	while(1){

		u16 	keys, down;
		int 	k;

		VBlankIntrWait();
		frame_start = cycles();

		// VRAM work first, while the screen isn't being drawn
		update_track();
		for(n = 0; n < NUM_CARS; n++) upload_car(n);
		place_cars();
		hud_race();
		// the page bit has no effect in mode 0; toggling it lets ojgbarun's fps= count frames
		REG_DISPCNT ^= BACKBUFFER;

		scanKeys();
		keys = keysHeld();
		down = keysDown();
		if(down & KEY_START){

			if(over){

				start_race();
				over = 0;
			}
			else D(___243ce0h) = !D(___243ce0h);
		}
		read_keys(keys, down);

		// the game's 70 Hz timer, for the vblanks since the last frame:
		// input into your car's controls, and the ticks race_logic() runs
		ticks_due += (vblanks - last_vblank)*TICKS_PER_SEC;
		last_vblank = vblanks;
		for(k = 0; (ticks_due >= VBLANKS_PER_SEC)&&(k < MAX_TICKS); k++){

			ticks_due -= VBLANKS_PER_SEC;
			race___40164h();
		}
		if(ticks_due >= VBLANKS_PER_SEC) ticks_due = VBLANKS_PER_SEC-1;	// fell behind: slow down instead of catching up

		if(!over){

			t = cycles();
			race_logic();
			t = cycles()-t;
			logic += t; ticks += k;
			if(t > logic_max) logic_max = t;

			t = cycles();
			race_render();
			race_render_after();
			render += cycles()-t;
			if((int)D(___2438c8h) > RACE_OVER) over = 1;
		}
		follow_camera();

		busy += cycles()-frame_start;
		frames++;

		if(vblanks - last_report >= VBLANKS_PER_SEC){

			char * 	p = line;

			// frames a second, and the CPU they took of the time there was
			memcpy(p, "FPS", 3); p = put_num(p + 3, frames*VBLANKS_PER_SEC/(vblanks - last_report), 3);
			memcpy(p, " CPU", 4); p = put_num(p + 4, (unsigned long long)busy*100/((vblanks - last_report)*FRAME_CYCLES), 3);
			*p++ = '%'; *p = 0;
			hud_print(0, 0, line);
			say("%s: logic %lu cycles a tick, at most %lu a frame; render %lu a frame",
				line, ticks ? logic/ticks : 0, logic_max, render/frames);
#if defined(RACE_TRACE)
			say("me %lu ai %lu", MY_CAR_IDX, D(___243ce0h));
			for(n = 0; n < NUM_OF_CARS; n++){

				say("car %d: %d,%d dir %d lap %d pos %d fin %lu", n, (int)car(n)->XLocation, (int)car(n)->YLocation,
					(int)car(n)->Direction, car(n)->Lap, car(n)->Position, car(n)->Finished);
			}
#endif
			last_report = vblanks;
			frames = 0; busy = logic = render = ticks = logic_max = 0;
		}
	}
}
