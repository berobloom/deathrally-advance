// Death Rally on the GBA: a race run by the game's own race code.
//
// race_logic() and race_render() (built with DR_RACE_NODRAW, so it keeps
// the camera, animations and lap times without drawing) run as on the
// desktop. The GBA draws the result its own way, as gba/proto does: the
// track as a tile background streamed from ROM, the cars as 64x64 sprites
// with their shadows through the OBJ window. race_setup() sets the race up
// as on the desktop, from the track and cars tools/gbabake baked into the ROM.
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
#include <gba_compression.h>
#include <gba_timers.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "drally.h"
#include "drally_keyboard.h"
#include "drally_structs_free.h"
#include "drally_race.h"
#include "drmath.h"
#include <maxmod.h>
#include "sound.h"
#include "race_replay.h"
#include "gba.h"

// baked by tools/gbabake, see assets.S
// each track's files, see assets.S
typedef struct {
	u32 		track;			// 0-9
	const u16 * 	pal;
	const u32 * 	inf;			// width and height in tiles, then in pixels
	const u8 * 	tiles, * mas, * setup_inf, * vai, * lr1;
	const u8 * 	ski, * blo, * lit, * varjo, * drv, * ohi;
	// reversed: rev_pal's size, 0 if the game doesn't reverse it, and the
	// colors and maps turned round; rev_vai and rev_lr1 are 0, the race
	// reads vai and lr1 turned round (race_qtr_last)
	u32 		reversible;
	const u16 * 	rev_pal;
	const u8 * 	rev_mas, * rev_vai, * rev_lr1;
} race_track_t;
extern const race_track_t race_tracks[];
extern const u32 race_tracks_n;
static const race_track_t * trk;	// the race's
extern const u8 setup_mines1a[], setup_blowi[], setup_obstacle[];
extern const u8 cars_img[];
extern const race_car_shapes_t cars_shp[];
extern const u8 cars_rev_img[];
extern const race_car_shapes_t cars_rev_shp[];
extern const u8 setup_obst_rev[];
_Static_assert(sizeof(race_car_shapes_t) == 8064, "assets.S's RACE_SHAPES_SIZE");
extern const u8 sprites_bin[];
extern const u8 hud_bin[];
extern const u32 shop_lz[];		// the shop's animations, see gbabake
extern const u16 menubg_dtl[];		// the menus' background, see gbabake's bake_menu_screen()
extern const u32 menufont_bin[];	// and their fonts
extern const u32 cursor_bin[];		// and cursor
extern const u8 menu_pal[];
extern const u8 panel_bin[];		// the side panel, see gbabake's bake_panel()
extern const u32 box_bin[];		// the menus' box, see gbabake's bake_box()
extern const u16 lamps_dtl[];		// a u16 for each frame's 96 tiles, then the tiles, see gbabake
#if defined(RACE_PRE)
extern const __DWORD__ race_pre[];	// race_main()'s arguments, the size of the globals, the globals
#endif
// make ALIGNED_CHECK=1: the word loads at unaligned addresses, see drally.h
extern unsigned dr_misaligned __attribute__((weak)), dr_misaligned_line __attribute__((weak));
extern const char * dr_misaligned_file __attribute__((weak));

void race_logic(void);
void race___40164h(void);

	extern __BYTE__ ___1e6ed0h[];
	extern __BYTE__ ___1de7d0h[];
	extern int NUM_OF_LAPS;
	extern __POINTER__ RACE_PEDESTR_BPK;
	extern __BYTE__ ___243ce0h[];
	extern __BYTE__ ___2438c8h[];
	extern __DWORD__ MY_CAR_IDX;
	extern __BYTE__ ___1de580h[];
	extern __BYTE__ ___243cdch[];
	extern __BYTE__ ___243cb8h[];
	extern __BYTE__ ___243ca0h[];
	extern int NUM_OF_CARS;
	extern int TRX_WIDTH;
	extern int TRX_HEIGHT;
	extern int X___243c8ch;
	extern int Y___243c90h;
	extern int CURRENT_VIEWPORT_W;
	extern int TRX_VIEWPORT_TL_X;
	extern int CURRENT_VIEWPORT_X;
	extern int CURRENT_VIEWPORT_CENTER_X;
	extern kb_control_t ___1a1140h;
	extern char ___19bd64h[16];
void race___400ech(void);

#define REG_WAITCNT		(*(vu16 *)0x04000204)
// WININ/WINOUT layer bits and BLDCNT fields, which libgba leaves out
#define WIN_BG0			(1 << 0)
#define WIN_BG1			(1 << 1)
#define WIN_BG2			(1 << 2)
#define WIN_OBJ			(1 << 4)
#define WIN_BLD			(1 << 5)
#define BLEND_BG0		(1 << 0)
#define BLEND_DARKEN		(3 << 6)
#define BLEND_BG0_UNDER		(1 << 8)	// BG0 as the second target
#define REG_DEBUG_ENABLE	(*(vu16 *)0x04FFF780)
#define REG_DEBUG_FLAGS		(*(vu16 *)0x04FFF700)
#define REG_DEBUG_STRING	((char *)0x04FFF600)

#define SCREEN_W		240
#define SCREEN_H		160
#define CAR_FRAMES		96
#define CAR_IMG_BYTES		(40*40)		// a rotation frame in the game's car images
#define CAR_OBJ_BYTES		(64*64)		// OBJ VRAM for each car's 64x64 sprite
#define CAR_W			40		// a car frame, 40x40
#define CAR_TILES		(CAR_W/8)	// 5x5 tiles from the top left of the sprite
#define CAR_FRAME_BYTES		(CAR_W*CAR_W)		// a frame in cars_img
#define NUM_CARS		4
// the car images in the ROM, assets.S's RACE_GBA_CARS: the 7 models,
// the Adversary's last, then the first 6's spiked ones (RACE_CAR_VARIANTS)
#define GBA_CAR_IMAGES		13
// the game's timer runs at 70 Hz, 7 ticks every 6 frames. Each tick runs
// race_logic() and race_render() once, as the game did on its 70 Hz VGA
// screen, and the vblank interrupt draws the last finished one, so a tick
// that runs into the next frame doesn't hold the screen up. make TICKS=60
// runs a tick a frame, at 6/7 of the game's speed
#if !defined(TICKS_PER_SEC)
#define TICKS_PER_SEC		70
#endif
#define VBLANKS_PER_SEC		60
#define FRAME_CYCLES		280896		// one 59.73 Hz frame of the 16.78 MHz CPU
// at most two ticks behind: after a longer hold-up the game slows down
// instead of racing to catch up
#define MAX_DUE			(3*VBLANKS_PER_SEC-1)
#define RACE_OVER		0x12c		// ___2438c8h counts up past this once the race is over

// the track background: a ring of 32x22 tiles in charblocks 0-2, map in screenblock 31
#define RING_W			32
#define RING_H			22
#define TRACK_MAP		((u16 *)SCREEN_BASE_BLOCK(31))
#define TRACK_CHARS		((u8 *)CHAR_BASE_BLOCK(0))
// the HUD: 8bpp glyphs in charblock 3, map in screenblock 30
#define HUD_MAP			((u16 *)SCREEN_BASE_BLOCK(30))
#define HUD_CHARS		((u8 *)CHAR_BASE_BLOCK(3))
// and the game's own box at the top left, 64x32: 8x4 of its tiles
#define BOX_TILE0		64	// after the glyphs
#define BOX_W			64
#define BOX_H			32
#if !defined(RACE_IMA_TILES)
#define RACE_IMA_TILES		1536	// the tiles the race can draw into, as race_load_baked.c's
#endif
// under it on BG2, its map in screenblock 29: a black tile, as BG1 shows its colour 0 as see-through
#define BOX_BACK_MAP		((u16 *)SCREEN_BASE_BLOCK(29))
#define BOX_BACK_TILE		(BOX_TILE0+(BOX_W/8)*(BOX_H/8))

// the keys race___40164h() reads, set in ___1a1140h
#define KEY_SCAN_ACCELERATE	DR_SCAN_A
#define KEY_SCAN_BRAKE		DR_SCAN_Z
#define KEY_SCAN_TURBO		DR_SCAN_S
#define KEY_SCAN_GUN		DR_SCAN_X
#define KEY_SCAN_MINE		DR_SCAN_SPACE
#define KEY_SCAN_HORN		DR_SCAN_H

// in EWRAM, like the other tables here that aren't read often: IWRAM
// is kept for the stack, which the vblank interrupt now adds to
EWRAM_BSS __BYTE__ kmap[256];

// the game's 70 Hz timer, which the race's animations go by: spectators,
// wrecks, explosions, turbo flames. Counted where race___40164h() runs
static __DWORD__ timer_ticks;
__DWORD__ __GET_FRAME_COUNTER(void){ return timer_ticks; }

EWRAM_BSS static __BYTE__ flat[0x4000] __attribute__((aligned(4)));
// the globals as they are at start-up: race_setup() leaves some of what a
// race changed, so every race starts from these, like the desktop's first
EWRAM_BSS static __BYTE__ flat_boot[0x4000] __attribute__((aligned(4)));
EWRAM_BSS static __BYTE__ obstacles[0x800] __attribute__((aligned(4)));

EWRAM_DATA static race_baked_t baked = {
	.mines1a = setup_mines1a, .blowi = setup_blowi, .obstacle = setup_obstacle,
	.cars = cars_img, .shapes = cars_shp, .sprites = sprites_bin, .hud = hud_bin,
	.obstacles = obstacles,
	.rev_cars = cars_rev_img, .rev_shapes = cars_rev_shp, .rev_obstacle = setup_obst_rev,
	.variants = GBA_CAR_IMAGES
};

static int track_w, track_h;		// in tiles
static int track_pw, track_ph;		// in pixels; the tiles are padded to a multiple of 8
static int cam_x, cam_y;
// the track turned round, see race_ima.c. RACE_CFLAGS=-DRACE_REVERSED: from
// the start, for the benchmark to compare with tools/replay --race Nr
#if defined(RACE_REVERSED)
static int reversed = 1;
#else
static int reversed;
#endif
static int ima_pad;			// and its image this far down its tiles
static int vis_x0 = -1, vis_y0, vis_x1, vis_y1;	// in its tiles
static int shown_frame[NUM_CARS];	// rotation frame in each car's OBJ VRAM
static const u8 * car_frames[NUM_CARS];	// each car's frames in cars_img
static volatile u32 vblanks;
EWRAM_BSS static u8 ring_row[256];		// ty % RING_H, without dividing
static int debug_log;
#if defined(RACE_BENCH)
static u32 bench_ticks, bench_max, bench_over190, bench_over200, bench_over225;
static unsigned long long bench_logic;
#endif

static void copy32(void * dst, const void * src, u32 bytes){

	REG_DMA3SAD = (u32)src;
	REG_DMA3DAD = (u32)dst;
	REG_DMA3CNT = DMA_ENABLE | DMA32 | (bytes >> 2);
}

// to mGBA's debug log
void say(const char * fmt, ...){

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

static void draw_view(void);
static void take_view(void);

static void vblank_handler(void){

	mmVBlank();		// first: it swaps the sound's buffers
	vblanks++;
	draw_view();
	mmFrame();		// the sound for the next frame
}

// --- track background ----------------------------------------------------

static void load_tile(int tx, int ty){

	int slot = (tx & (RING_W-1)) + ring_row[ty]*RING_W, turned;

	// as the race has drawn into it, or as it is in ROM, turned round by
	// the screen if the track is reversed
	copy32(TRACK_CHARS + 64*slot, race_ima_tile(tx, ty, &turned), 64);
	TRACK_MAP[(ty & 31)*32 + (tx & 31)] = slot | (turned ? 0x0c00 : 0);
}

// load the tiles that came into view since the last call, then scroll
static void update_track(int cam_x, int cam_y){

	cam_y += ima_pad;

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
static const char hud_chars[] = "0123456789FPSCU%GXLAREOV/TKNID<>HMBWJQYZ.$:-+!?',(&; ";
static const u8 hud_font[][7] = {
	{14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31}, {31,2,4,2,1,17,14},
	{2,6,10,18,31,2,2}, {31,16,30,1,1,17,14}, {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8},
	{14,17,17,14,17,17,14}, {14,17,17,15,1,2,12}, {31,16,16,30,16,16,16}, {30,17,17,30,16,16,16},
	{15,16,16,14,1,1,30}, {14,17,16,16,16,17,14}, {17,17,17,17,17,17,14}, {24,25,2,4,8,19,3},
	{14,17,16,23,17,17,15}, {17,17,10,4,10,17,17}, {16,16,16,16,16,16,31}, {14,17,17,31,17,17,17},
	{30,17,17,30,20,18,17}, {31,16,16,30,16,16,31}, {14,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
	{1,1,2,4,8,16,16}, {31,4,4,4,4,4,4}, {17,18,20,24,20,18,17}, {17,25,21,19,17,17,17},
	{14,4,4,4,4,4,14}, {30,17,17,17,17,17,30}, {2,4,8,16,8,4,2}, {8,4,2,1,2,4,8},
	{17,17,17,31,17,17,17}, {17,27,21,21,17,17,17}, {30,17,17,30,17,17,30}, {17,17,17,21,21,21,10},
	{7,2,2,2,2,18,12}, {14,17,17,17,21,18,13}, {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
	{0,0,0,0,0,12,12}, {4,15,20,14,5,30,4}, {0,12,12,0,12,12,0}, {0,0,0,31,0,0,0},
	{0,4,4,31,4,4,0}, {4,4,4,4,4,0,4}, {14,17,1,2,4,0,4}, {4,4,8,0,0,0,0},
	{0,0,0,0,12,4,8}, {2,4,8,8,8,4,2}, {12,18,20,8,21,18,13}, {0,12,12,0,12,4,8},
	{0,0,0,0,0,0,0}
};

EWRAM_BSS static u8 hud_glyph[128];		// glyph number of each character

static int closest_color(int r, int g, int b){

	int n, best = 1, best_d = 0x7fffffff;

	for(n = 1; n < 256; n++){

		int c = trk->pal[n], dr = (c & 31) - r, dg = ((c >> 5) & 31) - g, db = ((c >> 10) & 31) - b;
		int d = dr*dr + dg*dg + db*db;
		if(d < best_d){ best_d = d; best = n; }
	}

	return best;
}

// glyphs in white with a black outline, so they read on any part of the track
static void init_hud(void){

	int white = closest_color(31, 31, 31), black = closest_color(0, 0, 0);
	int g, x, y;

	// the race's layers, which the menus' screen changes
	REG_BG0CNT = BG_256_COLOR | CHAR_BASE(0) | SCREEN_BASE(31) | BG_SIZE_0 | BG_PRIORITY(2);
	REG_BG1CNT = BG_256_COLOR | CHAR_BASE(3) | SCREEN_BASE(30) | BG_SIZE_0 | BG_PRIORITY(0);
	REG_BG2CNT = BG_256_COLOR | CHAR_BASE(3) | SCREEN_BASE(29) | BG_SIZE_0 | BG_PRIORITY(0);
	REG_BG1HOFS = 0;
	REG_BG1VOFS = 0;

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
	for(g = 'a'; g <= 'z'; g++) hud_glyph[g] = hud_glyph[g-0x20];
	for(x = 0; x < 32*32; x++) HUD_MAP[x] = sizeof(hud_chars)-2;
	for(y = 0; y < BOX_H/8; y++) for(x = 0; x < BOX_W/8; x++) HUD_MAP[32*y+x] = BOX_TILE0+(BOX_W/8)*y+x;
	for(x = 0; x < BOX_W*BOX_H; x += 2) ((u16 *)(HUD_CHARS+64*BOX_TILE0))[x/2] = 0;
	for(x = 0; x < 64; x += 2) ((u16 *)(HUD_CHARS+64*BOX_BACK_TILE))[x/2] = black | (black << 8);
	for(x = 0; x < 32*32; x++) BOX_BACK_MAP[x] = sizeof(hud_chars)-2;
	for(y = 0; y < BOX_H/8; y++) for(x = 0; x < BOX_W/8; x++) BOX_BACK_MAP[32*y+x] = BOX_BACK_TILE;
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

// what the screen shows: the state after the last whole frame of the
// logic, taken by take_view() and drawn by the vblank interrupt
// the race's other pictures (race_sprite()), as OBJs of 8x8 or 16x16
#define MAX_SPRITES		32
#define SPRITE_TILE0		(NUM_CARS*CAR_OBJ_BYTES/32)	// their OBJ VRAM, after the cars'
typedef struct {
	const u8 * 	img;
	s16 		x, y;		// top left on the screen
	u8 		size;
} sprite_t;

typedef struct {
	int 	cam_x, cam_y;
	int 	x[NUM_CARS], y[NUM_CARS];	// the cars' centers on the screen
	int 	frame[NUM_CARS];		// their rotation frames
	int 	hidden[NUM_CARS];
	int 	lit[NUM_CARS];			// their headlights shown
	s16 	light[NUM_CARS][4];		// and turned, as OAM affine pa, pb, pc, pd
	int 	position, lap, over;
	int 	nspr, cars_at;			// sprites, and how many of them are under the cars
	sprite_t 	spr[MAX_SPRITES];		// in the order the race drew them
	const u16 * 	lamp;			// the start lights or the finish flag, its tiles in lamps_dtl, or 0
	int 	lamp_x;				// on the screen
	u32 	box_seq;			// the HUD box, in boxes[box_seq&1]
} view_t;

// what race_render() drew, in track coordinates
EWRAM_BSS static sprite_t drawn[MAX_SPRITES];
static int drawn_n, drawn_cars;
// and the start lights or the finish flag: 80x60, sliding in from the right
// of the screen
#define LAMP_W			0x50
#define LAMP_H			0x3c
// the headlights: a 64x64 16 color OBJ with the car's center in its
// middle, pointing up, 32 pixels to 40 on the screen
#define LIGHT_TILE0		(SPRITE_TILE0+8*MAX_SPRITES)
static int light_bank;			// its palette, make_light()'s
static int track_lit;			// whether the track's -LIT.TAB lightens anything
#define LAMP_TILE0		(LIGHT_TILE0+64)	// a 64x64 OBJ and a 32x64 one
#define LAMP_FRAMES		67
#define LAMP_TILES		96		// a frame's, in the 2 OBJs
static const u16 * drawn_lamp;		// its tiles in lamps_dtl
static int drawn_lamp_x;

static void on_sprite(const __BYTE__ * img, int w, int h, int x, int y){

	if((w == LAMP_W)&&(h == LAMP_H)){

		extern __POINTER__ GEN_LAM_BPK;
		extern __POINTER__ GEN_FLA_BPK;
		int 	f = ((img >= GEN_LAM_BPK)&&(img < GEN_LAM_BPK+3*0x12c0)) ? (img-GEN_LAM_BPK)/0x12c0 : 3+(img-GEN_FLA_BPK)/0x12c0;

		// on the game's 320 pixel wide screen, at its right edge on ours
		drawn_lamp = lamps_dtl+LAMP_TILES*f;
		drawn_lamp_x = x-TRX_VIEWPORT_TL_X+CURRENT_VIEWPORT_X-(320-SCREEN_W);
		return;
	}
	if(!img){

		drawn_cars = drawn_n;
		return;
	}
	if((drawn_n == MAX_SPRITES)||(w != h)||((w != 8)&&(w != 16))) return;
	drawn[drawn_n].img = img;
	drawn[drawn_n].x = x;
	drawn[drawn_n].y = y;
	drawn[drawn_n].size = w;
	drawn_n++;
}

static void draw_box(void);

// race_render(), keeping what it draws for the next view
static void render_race(void){

	drawn_n = drawn_cars = 0;
	drawn_lamp = 0;
	race_render();
	draw_box();
}

EWRAM_BSS static view_t views[2];
static int view_next;			// the one take_view() fills
static volatile int view_ready = -1;	// the one to draw at the next vblank
static volatile int view_drawing;	// 0 while a race is set up
static volatile u32 views_drawn, isr_cycles;

// the box the wide view's HUD has at the top left (race___40864h()): the
// game draws it into BACKBUFFER, here flat[], which nothing needs while the
// race runs. Its rows are 0x200 apart and start at column 0x60
extern __POINTER__ game_backbuffer __asm__("BACKBUFFER");
void race___40864h(void);
static u32 box_seq;
static int box_wait;		// set: not now, the logic is behind
// as 8x8 tiles: the newest, and the one before, which a view not yet drawn may show
EWRAM_BSS static u8 boxes[2][BOX_W*BOX_H] __attribute__((aligned(4)));

static void draw_box(void){

	static u32 	drawn_key[8], drawn_at = -1;
	const __BYTE__ * 	c = ___1de580h+0x94*MY_CAR_IDX;
	u32 	key[8], * dst;
	int 	tx, ty, y;

	// only when what it shows changes, 30 times a second at most, and not
	// while the logic is behind:
	// the turbo bar, the damage, and the lap time, or the ammo bar and the
	// mines, as race___40864h() works them out
	key[0] = (int)((57.0/1024.0/100.0)*(double)(int)D(c+0x34));
	if((int)key[0] < 0) key[0] = 0;
	key[1] = (int)(100.0-dRMath_ceil((1.0/1024.0)*(double)(int)D(c+0x18)));
	key[2] = D(c+0x2c);
	if(key[2] == 0){

		key[3] = (int)D(___243cdch) > 0;
		key[4] = key[3] ? D(___243cb8h) : D(___243ca0h);
		key[5] = key[6] = 0;
	}
	else {

		key[3] = key[4] = 0;
		key[5] = (int)((11.0/2048.0/10.0)*(double)(int)D(c+0x30));
		if((int)key[5] < 0) key[5] = 0;
		key[6] = D(c+0x28);
	}
	key[7] = MY_CAR_IDX;		// whose box: another race's may have another car
	if(box_wait||(vblanks-drawn_at < 2)||!memcmp(key, drawn_key, sizeof(key))){

		views[view_next].box_seq = box_seq;
		return;
	}
	memcpy(drawn_key, key, sizeof(key));
	drawn_at = vblanks;

	game_backbuffer = (__POINTER__)((u32)flat-0x60);
	race___40864h();
	dst = (u32 *)boxes[(box_seq+1)&1];
	for(ty = 0; ty < BOX_H/8; ty++){
		for(tx = 0; tx < BOX_W/8; tx++){
			for(y = 0; y < 8; y++){

				const u32 * 	src = (const u32 *)(flat+0x200*(8*ty+y)+8*tx);

				dst[0] = src[0];
				dst[1] = src[1];
				dst += 2;
			}
		}
	}
	views[view_next].box_seq = ++box_seq;
}

// in the vblank interrupt
static void place_box(const view_t * v){

	static u32 	shown;

	if(v->box_seq == shown) return;
	copy32(HUD_CHARS+64*BOX_TILE0, boxes[v->box_seq&1], sizeof(boxes[0]));
	shown = v->box_seq;
}

// a car's 40x40 frame, as the race reads it (cars_img, in place 0's body
// colors), into the 8bpp tiles of the 64x64 sprite at dst, with add added
// to its body colors. In ARM in IWRAM: the vblank does it for each car
// that turned
static IWRAM_CODE ARM_CODE __attribute__((noinline)) void tile_car(u32 * dst, const u32 * src, u32 add){

	int 	y, x;

	for(y = 0; y < CAR_W; y++){

		// row y&7 of the tiles in tile row y >> 3, which are 8 wide
		u32 	* d = dst+(y >> 3)*8*16+(y&7)*2;

		for(x = 0; x < 2*CAR_TILES; x++){

			// each byte's top bit: 15 or more, 25 or more, of the low 7
			// bits, then those of 15 to 24 as a mask of the whole byte.
			// None carries: the colors are at most 24, the most added 30
			u32 	w = *src++, l = w&0x7f7f7f7f;
			u32 	in = (l+0x71717171)&~(l+0x67676767)&~w&0x80808080;

			d[16*(x >> 1)+(x&1)] = w+(add&((in >> 7)*0xff));
		}
	}
}

static void upload_car(int n, const view_t * v){

	int 	frame = v->frame[n];

	if((frame == shown_frame[n])||v->hidden[n]) return;
	// the first car's as they are, the others' with their own body colors
	// (race___42824h()'s): 15 to 24 moved up 10 a place
	tile_car((u32 *)((u8 *)SPRITE_GFX + n*CAR_OBJ_BYTES), (const u32 *)(car_frames[n] + frame*CAR_FRAME_BYTES), 0x0a0a0a0a*n);
	shown_frame[n] = frame;
}

// sprite k of the view into OAM entry o, its image into its OBJ VRAM if
// that holds another
static const u8 * sprite_shown[MAX_SPRITES];	// the image in each sprite's OBJ VRAM
static const u16 * lamp_shown;			// and in the start lights'

static void place_sprite(int o, int k, const sprite_t * s){

	const u8 ** 		shown = sprite_shown;
	int 			tile = SPRITE_TILE0+8*k;

	if(shown[k] != s->img){

		// 8bpp tiles, in rows of tiles: 8 bytes a row of a tile
		u32 * 		dst = (u32 *)((u8 *)SPRITE_GFX+32*tile);
		const u32 * 	src = (const u32 *)s->img;
		int 		tx, ty, r, n = s->size >> 3, w = s->size >> 2;

		for(ty = 0; ty < n; ty++) for(tx = 0; tx < n; tx++) for(r = 0; r < 8; r++){

			*dst++ = src[(8*ty+r)*w+2*tx];
			*dst++ = src[(8*ty+r)*w+2*tx+1];
		}
		shown[k] = s->img;
	}
	OAM[o].attr0 = OBJ_Y(s->y) | OBJ_256_COLOR | OBJ_SQUARE;
	// race___4af3ch() turns the spectators' pictures round on a reversed track
	OAM[o].attr1 = OBJ_X(s->x) | OBJ_SIZE((s->size == 16) ? 1 : 0)
		| ((reversed&&(s->img >= RACE_PEDESTR_BPK)&&(s->img < RACE_PEDESTR_BPK+0x2400)) ? OBJ_HFLIP|OBJ_VFLIP : 0);
	OAM[o].attr2 = OBJ_CHAR(tile) | OBJ_PRIORITY(1);
}

// the start lights as a 64x64 OBJ and a 32x64 one, over everything
static int place_lamp(const view_t * v){

	if(!v->lamp||(v->lamp_x >= SCREEN_W)) return 0;
	if(lamp_shown != v->lamp){

		const u8 	* tiles = (const u8 *)(lamps_dtl+LAMP_FRAMES*LAMP_TILES);
		int 		k;

		for(k = 0; k < LAMP_TILES; k++) copy32((u8 *)SPRITE_GFX+32*LAMP_TILE0+64*k, tiles+64*v->lamp[k], 64);
		lamp_shown = v->lamp;
	}
	OAM[0].attr0 = OBJ_Y(0) | OBJ_256_COLOR | OBJ_SQUARE;
	OAM[0].attr1 = OBJ_X(v->lamp_x) | OBJ_SIZE(3);
	OAM[0].attr2 = OBJ_CHAR(LAMP_TILE0) | OBJ_PRIORITY(0);
	OAM[1].attr0 = OBJ_Y(0) | OBJ_256_COLOR | OBJ_TALL;
	OAM[1].attr1 = OBJ_X(v->lamp_x+64) | OBJ_SIZE(3);
	OAM[1].attr2 = OBJ_CHAR(LAMP_TILE0+128) | OBJ_PRIORITY(0);

	return 2;
}

// the OAM: the start lights, the race's pictures drawn after the cars,
// the last one on top, then the cars, then the pictures drawn before them
static void place_objs(const view_t * v){

	static int 	used;
	int 		n, k, o = place_lamp(v);

	for(k = v->nspr-1; k >= v->cars_at; k--) place_sprite(o++, k, &v->spr[k]);
	for(n = 0; n < NUM_CARS; n++, o += 2){

		int sx = v->x[n] - 20, sy = v->y[n] - 20;
		int tile = n*CAR_OBJ_BYTES/32;
		int hidden = v->hidden[n];

		// the car, then its shadow as OBJ window 5 pixels down and to the right
		OAM[o].attr0 = hidden ? OBJ_DISABLE : (OBJ_Y(sy) | OBJ_256_COLOR | OBJ_SQUARE);
		OAM[o].attr1 = OBJ_X(sx) | OBJ_SIZE(3);
		OAM[o].attr2 = OBJ_CHAR(tile) | OBJ_PRIORITY(1);
		OAM[o+1].attr0 = hidden ? OBJ_DISABLE : (OBJ_Y(sy+5) | OBJ_256_COLOR | OBJ_SQUARE | OBJ_MODE(2));
		OAM[o+1].attr1 = OBJ_X(sx+5) | OBJ_SIZE(3);
		OAM[o+1].attr2 = OBJ_CHAR(tile) | OBJ_PRIORITY(1);
	}
	for(k = v->cars_at-1; k >= 0; k--) place_sprite(o++, k, &v->spr[k]);
	// the headlights under all of it: each a semi-transparent OBJ, which
	// blends only inside the OBJ window, so a copy of it there too
	for(n = 0; n < NUM_CARS; n++){

		OBJAFFINE 	* a = &((OBJAFFINE *)OAM)[n];
		int 		k, sx = v->x[n] - 64, sy = v->y[n] - 64;

		if(!v->lit[n]) continue;
		a->pa = v->light[n][0];
		a->pb = v->light[n][1];
		a->pc = v->light[n][2];
		a->pd = v->light[n][3];
		for(k = 1; k <= 2; k++, o++){

			OAM[o].attr0 = OBJ_Y(sy) | OBJ_ROT_SCALE_ON | OBJ_DOUBLE | OBJ_16_COLOR | OBJ_SQUARE | OBJ_MODE(k);
			OAM[o].attr1 = OBJ_X(sx) | OBJ_ROT_SCALE(n) | OBJ_SIZE(3);
			OAM[o].attr2 = OBJ_CHAR(LIGHT_TILE0) | OBJ_PRIORITY(1) | OBJ_PALETTE(light_bank);
		}
	}
	for(n = o; n < used; n++) OAM[n].attr0 = OBJ_DISABLE;
	used = o;
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

// the track the race and the screen use from now on
static void use_track(const race_track_t * t){

	trk = t;
	baked.inf = t->setup_inf;
	baked.mas = t->mas;
	baked.vai = t->vai;
	baked.lr1 = t->lr1;
	baked.ski_tab = t->ski;
	baked.blo_tab = t->blo;
	baked.lit_tab = t->lit;
	baked.varjo_tab = t->varjo;
	baked.drv_dat = t->drv;
	baked.ohi_dat = t->ohi;
	baked.tiles = t->tiles;
	baked.rev_mas = t->reversible ? t->rev_mas : 0;
	baked.rev_vai = t->rev_vai;
	baked.rev_lr1 = t->rev_lr1;
	if(!t->reversible) reversed = 0;
	ima_pad = 0;
	track_w = t->inf[0];
	track_h = t->inf[1];
	track_pw = t->inf[2];
	track_ph = t->inf[3];
	copy32(BG_PALETTE, reversed ? t->rev_pal : t->pal, 512);
	copy32(SPRITE_PALETTE, reversed ? t->rev_pal : t->pal, 512);
	init_hud();			// its colours are the track's
	// what was on screen is another track's, and so are the tiles drawn into
	vis_x0 = -1;
	race_ima_tiles(t->tiles, track_w, track_h, RACE_IMA_TILES);
}

// what you race with: your car (0-5, the game's VAGABOND to DELIVERATOR),
// the laps, the others' difficulty (0-2, SPEED MAKES ME DIZZY to PETROL IN
// MY VEINS) and your car's upgrades. The defaults are race_quick()'s
#define NUM_MODELS		6
// and whether there are weapons: guns and mines, for everyone or no one.
// RACE_CFLAGS=-DRACE_NO_WEAPONS: none from the start, for the benchmark
#if defined(RACE_NO_WEAPONS)
static int weapons = 0;
#else
static int weapons = 1;
#endif
static int my_model = 3, laps = 4, rivals = 1, upgrades = 0;
static const char * const model_names[NUM_MODELS] = { "VAGABOND", "DERVISH", "SENTINEL", "SHRIEKER", "WRAITH", "DELIVERATOR" };

// the engine and tires each car can be fitted with: race___3f970h()'s
// tables have 1.00 in the places past them, which would leave the car
// crawling. Armor goes up to 4 on all of them
static const u8 max_engine[NUM_MODELS] = { 1, 2, 2, 3, 3, 4 };
static const u8 max_tires[NUM_MODELS] = { 2, 2, 3, 3, 4, 4 };

// race_quick()'s quick race, with the choices put in
static void apply_choices(void){

	__BYTE__ * 	r;
	int 		n;

	NUM_OF_LAPS = laps;
	D(___1de7d0h+0x48) = reversed;
	for(n = 1; n < NUM_CARS; n++) D(___1de7d0h+0x54*n+0x44) = rivals;
	for(n = 0; n < NUM_CARS; n++) D(___1de7d0h+0x54*n+0x2c) = weapons;
	r = ___1de7d0h;
	D(r+0x14) = (upgrades < max_engine[my_model]) ? upgrades : max_engine[my_model];
	D(r+0x18) = (upgrades < max_tires[my_model]) ? upgrades : max_tires[my_model];
	D(r+0x1c) = upgrades;
}

// one row of choose_race(), centred, with arrows if it's the one chosen
static void choice_row(int row, int chosen, const char * label, const char * value){

	char 	line[32];
	int 	n = 0;

	line[n++] = (row == chosen) ? '<' : ' ';
	line[n++] = ' ';
	while(*label) line[n++] = *label++;
	if(n > 2) line[n++] = ' ';
	while(*value) line[n++] = *value++;
	line[n++] = ' ';
	line[n++] = (row == chosen) ? '>' : ' ';
	line[n] = 0;
	hud_print((SCREEN_W/8-n)/2, (row < 2) ? 1+2*row : 13+row, line);
}

// before a race: up and down pick a row, left and right change it (the
// track pans by, your car turns in the middle), and A or Start races
#define CHOICES 		6
static int choose_race(void){

	int 	k = trk-race_tracks, x = 0, y = 0, dx = 1, dy = 1, row = 0, n;
	char 	num[8];

#if defined(RACE_BENCH)
	return 0;		// the first track, with nobody to choose
#endif
	view_drawing = 0;
	sound_music(-1);
	REG_DISPCNT = MODE_0 | BG0_ON | BG1_ON | OBJ_ON | OBJ_1D_MAP;
	for(n = 0; n < 128; n++) OAM[n].attr0 = OBJ_DISABLE;
	// your car, turning in the middle of the screen in car 0's sprite
	OAM[0].attr0 = OBJ_Y(SCREEN_H/2-32) | OBJ_256_COLOR | OBJ_SQUARE;
	OAM[0].attr1 = OBJ_X(SCREEN_W/2-32) | OBJ_SIZE(3);
	OAM[0].attr2 = OBJ_CHAR(0) | OBJ_PRIORITY(0);
	for(;;){

		const u8 * 	frames;

		use_track(&race_tracks[k]);
		if(reversed){

			// turned round as the race will have it; race_setup() sets
			// TRX_HEIGHT, which the turned tiles go by, again
			TRX_HEIGHT = track_ph;
			race_ima_reverse();
			ima_pad = race_ima_pad();
		}
		x = (track_pw-SCREEN_W)/2;
		y = (track_ph-SCREEN_H)/2;
		for(;;){

			for(n = 0; n < 32*32; n++) HUD_MAP[n] = sizeof(hud_chars)-2;
			*put_num(num, trk->track+1, 2) = 0;
			if(reversed) strcat(num, " REV");
			choice_row(0, row, "TRACK", num);
			choice_row(1, row, "", model_names[my_model]);
			*put_num(num, laps, 1) = 0;
			choice_row(2, row, "LAPS", num);
			*put_num(num, rivals+1, 1) = 0;
			choice_row(3, row, "RIVALS", num);
			*put_num(num, upgrades, 1) = 0;
			choice_row(4, row, "UPGRADES", num);
			choice_row(5, row, "WEAPONS", weapons ? "ON" : "OFF");
			hud_print((SCREEN_W/8-7)/2, 19, "PRESS A");

			// the track going by slowly
			VBlankIntrWait();
			if((x+dx < 0)||(x+dx > track_pw-SCREEN_W)) dx = -dx;
			if((y+dy < 0)||(y+dy > track_ph-SCREEN_H)) dy = -dy;
			x += dx;
			if(vblanks&1) y += dy;
			update_track(x, y);
			frames = cars_img+my_model*CAR_FRAMES*CAR_FRAME_BYTES+((vblanks/3)%CAR_FRAMES)*CAR_FRAME_BYTES;
			tile_car((u32 *)SPRITE_GFX, (const u32 *)frames, 0);

			scanKeys();
			n = keysDown();
			if(n&(KEY_A|KEY_START|KEY_B)){

				OAM[0].attr0 = OBJ_DISABLE;
				init_hud();		// the race's
				return !!(n&KEY_B);	// B: back to the title
			}
			if(n&KEY_UP) row = (row+CHOICES-1)%CHOICES;
			if(n&KEY_DOWN) row = (row+1)%CHOICES;
			n = (n&KEY_RIGHT) ? 1 : (n&KEY_LEFT) ? -1 : 0;
			if(!n) continue;
			if(row == 0){

				// each track, then reversed if the game reverses it
				if((n > 0)&&!reversed&&race_tracks[k].reversible) reversed = 1;
				else if((n < 0)&&reversed) reversed = 0;
				else {

					k = (k+race_tracks_n+n)%race_tracks_n;
					reversed = (n < 0)&&race_tracks[k].reversible;
				}
				break;
			}
			if(row == 1) my_model = (my_model+NUM_MODELS+n)%NUM_MODELS;
			if(row == 2) laps = (laps+n+8)%9+1;		// 1-9
			if(row == 3) rivals = (rivals+n+3)%3;
			if(row == 4) upgrades = (upgrades+n+5)%5;
			if(row == 5) weapons = !weapons;
		}
	}
}

// the race: a quick one with what choose_race() chose, or with career
// set the career's, which ___1de7d0h and the rest hold already (see
// career_race() in career.c), with you in place my_index of num_cars
// the cars' body colors, 10 from 15+10*n for car n, which race___4a25ch()
// built into the game's palette from each driver's color, the Adversary's
// car's aside: gbabake baked a quick race's four
static void car_colors(int num_cars){

	extern __BYTE__ ___1a51d0h[];

	int 	n, i;

	for(n = 0; n < num_cars; n++){

		if((n == 0)&&(D(___1de580h) == 6)) continue;
		for(i = 15+10*n; i < 25+10*n; i++){

			const __BYTE__ 	* c = ___1a51d0h+3*i;
			u16 		v = (c[0] >> 1)|((c[1] >> 1) << 5)|((c[2] >> 1) << 10);

			BG_PALETTE[i] = v;
			SPRITE_PALETTE[i] = v;
		}
	}
}

// the headlights' OBJ: race___4f300h()'s three triangles from the car's
// center, to 40 pixels at 10 degrees each side and 36 at 18. The tracks'
// -LIT.TABs light in their own tones, from TR0's pale blue to TR5's red, so it
// adds what the track's adds to its colors on average: as a color none of
// the cars' (car_colors()) take, times as little of it as will do

static void make_light(void){

	// the outline, x16, pointing up from (0, 0)
	static const s16 	poly[5][2] = { {0, 0}, {-178, -548}, {-111, -630}, {111, -630}, {178, -548} };
	u32 			* dst = (u32 *)((u8 *)SPRITE_GFX+32*LIGHT_TILE0);
	extern __BYTE__ 	TRX_LIT_TAB[0x100];
	int 			n, best = 1, best_d = 0x7fffffff, tx, ty, r, x, c;
	int 			add[3] = { 0, 0, 0 }, lit = 0, eva = 1, k;

	for(n = 0; n < 256; n++) if(TRX_LIT_TAB[n] != n){

		int 	a = trk->pal[n], b = trk->pal[TRX_LIT_TAB[n]];

		add[0] += (b & 31)-(a & 31);
		add[1] += ((b >> 5) & 31)-((a >> 5) & 31);
		add[2] += ((b >> 10) & 31)-((a >> 10) & 31);
		lit++;
	}
	// three of the tracks' leave every color as it is
	if(!(track_lit = (lit > 0))) return;
	for(n = 0; n < 3; n++) add[n] = 16*add[n]/lit;		// x16, as the blending adds color*eva/16
	for(k = 1; k <= 16; k++) for(n = 1; n < 256; n++){

		int 	p = trk->pal[n], dr = (p & 31)*k-add[0], dg = ((p >> 5) & 31)*k-add[1], db = ((p >> 10) & 31)*k-add[2];
		int 	d = dr*dr+dg*dg+db*db;

		if(!(n & 15)||((n >= 15)&&(n < 15+10*NUM_CARS))) continue;
		if(d < best_d){ best_d = d; best = n; eva = k; }
	}
	REG_BLDALPHA = eva | (16 << 8);
	light_bank = best >> 4;
	c = best & 15;
	// 4bpp tiles, 8 of them a row, 4 bytes a row of a tile
	for(ty = 0; ty < 8; ty++) for(tx = 0; tx < 8; tx++) for(r = 0; r < 8; r++){

		u32 	row = 0;

		for(x = 0; x < 8; x++){

			// the texel's center, in screen pixels x16
			int 	px = (2*(8*tx+x)+1-64)*10, py = (2*(8*ty+r)+1-64)*10;
			int 	in = 1;

			for(n = 0; in&&(n < 5); n++){

				const s16 	* a = poly[n], * b = poly[(n+1)%5];

				in = (b[0]-a[0])*(py-a[1])-(b[1]-a[1])*(px-a[0]) >= 0;
			}
			if(in) row |= (u32)c << (4*x);
		}
		*dst++ = row;
	}
}

static const char * start_race(int career, int my_index, int num_cars){

	int 		n;

	view_drawing = 0;
	view_ready = -1;
	timer_ticks = 0;
	if(race_replay_size() > sizeof(flat)) return "flat[] too small";
	// a quick race from the globals as they were at start-up; the
	// career's from those its last race left, as on the desktop (see
	// gba_career_reset())
	if(!career) race_replay_scatter(flat_boot);
#if defined(RACE_PRE)
	// as the desktop's menus left it
	if(race_pre[2] != race_replay_size()) return "pre.bin is from another build";
	my_index = race_pre[0];
	num_cars = race_pre[1];
	race_replay_scatter((const __BYTE__ *)(race_pre+3));
#else
	if(career){

		// the track the career's race is on, turned round or not
		for(n = 0; (n < (int)race_tracks_n)&&(race_tracks[n].track != (u32)(___19bd64h[2]-'0')); n++);
		if(n == (int)race_tracks_n) return "the career's track isn't in the ROM";
		reversed = D(___1de7d0h+0x48);
		// the menus' layers off while it's set up: their tiles are in
		// the menus' colors, which use_track() changes
		REG_DISPCNT = MODE_0;
		use_track(&race_tracks[n]);
		say("career race on %s%s, %d laps, you in place %d of %d", ___19bd64h, reversed ? " reversed" : "", NUM_OF_LAPS, my_index, num_cars);
	}
	else {
		// gbabake's car colors (track.pal), red, blue, yellow and green
		// yours, then a DELIVERATOR, a WRAITH and a SENTINEL
		int 			models[NUM_CARS] = { my_model, 5, 4, 2 };
		static const __BYTE__ 	colors[NUM_CARS][3] = { {63, 8, 4}, {8, 24, 63}, {63, 52, 4}, {8, 52, 12} };
		// RACE_CFLAGS=-DRACE_SEED=n: always the same race, to compare its
		// hash with tools/replay --quick
#if defined(RACE_SEED)
		__DWORD__ 		seed = RACE_SEED;
#else
		__DWORD__ 		seed = cycles();
#endif

		race_quick(trk->track, models, colors, seed);
		apply_choices();
		say("quick race on TR%lu%s, seed %lu, %d laps, rivals %d, upgrades %d, weapons %d", trk->track, reversed ? " reversed" : "", seed, laps, rivals, upgrades, weapons);
		my_index = 0;
		num_cars = NUM_CARS;
	}
#endif

	race_baked = &baked;
	race_sprite_cb = on_sprite;
	race_setup(my_index, num_cars, &race_loaders_baked);
	ima_pad = race_ima_pad();
	car_colors(num_cars);
	// the menus' pictures and side panel out of OBJ VRAM and OAM: a car
	// fills only the top left 40x40 of its 64x64 sprite, the rest of it
	// would show what was there
	for(n = 0; n < 128; n++) OAM[n].attr0 = OBJ_DISABLE;
	for(n = 0; n < 0x8000/4; n++) ((u32 *)SPRITE_GFX)[n] = 0;
	for(n = 0; n < MAX_SPRITES; n++) sprite_shown[n] = 0;
	lamp_shown = 0;
	make_light();
	race_replay_gather(flat);
	say("set up: globals hash %08lx", race_replay_hash(flat));
	// RACE_CFLAGS=-DVAR_HASHES: each global's, as VAR_HASHES=1 tools/replay --quick prints them
#if defined(VAR_HASHES)
	{
		unsigned 	i, o, k;
		__DWORD__ 	h;

		for(i = o = 0; i < race_replay_count(); o += race_replay_var_size(i++)){
			for(h = 0x811c9dc5, k = 0; k < race_replay_var_size(i); k++) h = (h^flat[o+k])*0x01000193;
			say("var %s %08lx", race_replay_var_name(i), h);
		}
	}
#endif

	for(n = 0; n < NUM_CARS; n++){

		car_frames[n] = cars_img + ((n < NUM_OF_CARS) ? race_car_variant(n) : 0)*CAR_FRAMES*CAR_FRAME_BYTES;
	}

	// the track's music, and your engine, as race_main() starts them
	sound_music(trk-race_tracks);
	dRally_Sound_pushEffect(1, (D(___1de580h+0x94*MY_CAR_IDX)&0xff)+25, 0, 0x10000, 0x28000, 0x8000);

#if defined(RACE_BENCH)
	D(___243ce0h) = 1;	// the AI drives your car too
#else
	D(___243ce0h) = 0;	// you drive, not the AI
#endif
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
	render_race();
	follow_camera();
	take_view();
	view_drawing = 1;

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

static void hud_race(const view_t * v){

	char 	line[16], * p = line;

	*p++ = 'P'; p = put_num(p, v->position, 1);
	*p++ = ' '; *p++ = 'L'; p = put_num(p, v->lap, 2);
	*p = 0;
	hud_print(0, 19, line);
	hud_print(10, 9, v->over ? "RACE OVER" : "         ");
}

// the state the logic left, for the next vblank to draw
static void take_view(void){

	view_t * 	v = &views[view_next];
	int 		n;

	v->cam_x = cam_x;
	v->cam_y = cam_y;
	for(n = 0; n < NUM_CARS; n++){

		v->x[n] = (int)car(n)->XLocation - cam_x;
		v->y[n] = (int)car(n)->YLocation - cam_y;
		v->frame[n] = dRMath_div15(car(n)->ImgOffset >> 6, CAR_IMG_BYTES >> 6)-n*CAR_FRAMES;	// ImgOffset/CAR_IMG_BYTES
		v->hidden[n] = (n >= NUM_OF_CARS)||(v->x[n] <= -32)||(v->x[n] >= SCREEN_W+32)||(v->y[n] <= -32)||(v->y[n] >= SCREEN_H+32);
		// race___4f300h()'s, for a car not wrecked and still racing
		v->lit[n] = track_lit&&(n < NUM_OF_CARS)&&((int)D(___1de580h+0x94*n+0x18) > 0)&&!car(n)->Finished
			&&(v->x[n] > -40)&&(v->x[n] < SCREEN_W+40)&&(v->y[n] > -40)&&(v->y[n] < SCREEN_H+40);
		if(v->lit[n]){

			// Direction+180 ahead, x by its sine and y by its cosine, y
			// scaled by 5/6 as the game does it for its screen's pixels
			int 	t = dRMath_turns_degf(car(n)->Direction);
			int 	s = dRMath_sin_i(t) >> 20, c = dRMath_cos_i(t) >> 20;	// 8.8

			v->light[n][0] = (205*c) >> 8;		// 0.8 cos
			v->light[n][1] = -((246*s) >> 8);	// -0.96 sin
			v->light[n][2] = (205*s) >> 8;		// 0.8 sin
			v->light[n][3] = (246*c) >> 8;		// 0.96 cos
		}
	}
	// the pictures on screen
	v->nspr = v->cars_at = 0;
	for(n = 0; n < drawn_n; n++){

		sprite_t 	s = drawn[n];

		s.x -= cam_x;
		s.y -= cam_y;
		if((s.x <= -s.size)||(s.x >= SCREEN_W)||(s.y <= -s.size)||(s.y >= SCREEN_H)) continue;
		v->spr[v->nspr++] = s;
		if(n < drawn_cars) v->cars_at++;
	}
	v->lamp = drawn_lamp;
	v->lamp_x = drawn_lamp_x;
	v->position = car(MY_CAR_IDX)->Position;
	v->lap = car(MY_CAR_IDX)->Lap;
	v->over = (int)D(___2438c8h) > 0;
	// the interrupt only reads views, so this needs no lock: it draws
	// either the one before or this one
	view_ready = view_next;
	view_next ^= 1;
}

// in the vblank interrupt: VRAM work while the screen isn't being drawn
static void draw_view(void){

	const view_t * 	v;
	u32 		t;
	int 		n;

	if(!view_drawing||(view_ready < 0)) return;
	t = cycles();
	v = &views[view_ready];
	view_ready = -1;
	update_track(v->cam_x, v->cam_y);
	// the tiles on screen that the race drew into
	while((n = race_ima_dirty()) != -1){

		int 	tx = n&0xff, ty = n >> 8;

		if(n == -2){

			vis_x0 = -1;
			update_track(v->cam_x, v->cam_y);
		}
		else if((tx >= vis_x0)&&(tx <= vis_x1)&&(ty >= vis_y0)&&(ty <= vis_y1)) load_tile(tx, ty);
	}
	for(n = 0; n < NUM_CARS; n++) upload_car(n, v);
	place_objs(v);
	place_box(v);
	hud_race(v);
	// the page bit has no effect in mode 0; toggling it lets ojgbarun's fps= count frames
	REG_DISPCNT ^= BACKBUFFER;
	views_drawn++;
	isr_cycles += cycles()-t;
}

// --- text screens --------------------------------------------------------------

u32 gba_cycles(void){ return cycles(); }

static int shown_model = -1;	// gba_show_car()'s
// gba_show_part()'s: its frames, how many, and whether it goes back and forth
static const u32 * shown_part;		// its frames' places in shop_lz
static int shown_part_frame;
static int part_frames, part_bounce;

// The menus' screen, as the desktop's: its background with the DEATH
// RALLY banner on BG3, 8bpp in menu.pal's colors, and over it on BG1 the
// text, drawn in the desktop's fonts at half their size into 4bpp tiles,
// one for each 8x8 of the screen. The fonts are in menu.pal's grey ramp,
// colors 1-15, which a tile's palette bank can make another of its ramps.
// The race's tiles and maps are where these are; use_track()'s init_hud()
// puts its layers back
#define MENU_BG_MAP		((u16 *)SCREEN_BASE_BLOCK(19))
#define MENU_TEXT_MAP		((u16 *)SCREEN_BASE_BLOCK(20))
#define MENU_BOX_MAP		((u16 *)SCREEN_BASE_BLOCK(21))
#define MENU_TEXT_TILE0		384		// from charblock 2, after the maps
#define MENU_TEXT		((u32 *)CHAR_BASE_BLOCK(2)+8*MENU_TEXT_TILE0)
#define MENU_BLANK_TILE		(MENU_TEXT_TILE0+30*20)
// the boxes' tiles (gbabake's box.bin) after the blank one, from charblock 3
#define MENU_BOX_TILE0		((32*(MENU_BLANK_TILE+1)-0x4000)/32)
#define MENU_BOX_BANK		15
#define MENU_DISPCNT		(MODE_0 | BG1_ON | BG2_ON | BG3_ON | OBJ_ON | OBJ_1D_MAP)
#define FONT_SMALL		menufont_bin
#define FONT_BIG		(menufont_bin+96*8)
#define FONT_WIDTHS		((const u8 *)(menufont_bin+96*8+96*32))

static int text_bank, text_shade = 15;		// gba_text_color()'s
// what's printed in each row: a hash of each string, its color and where
// it is, and the cells it took, so that printing it again does nothing
EWRAM_BSS static u32 text_was[20][30];
EWRAM_BSS static u8 text_was_n[20][30];

// whether s at col is in row already; if not, the strings it covers are
// forgotten and it's noted
static int text_known(int col, int row, int cells, const char * s){

	u32 	h = 0x811c9dc5^(text_bank << 8)^text_shade;
	int 	c;

	if((row < 0)||(row >= 20)||(col < 0)||(col >= 30)) return 0;
	for(; *s; s++) h = (h^(u8)*s)*0x01000193;
	if(!h) h = 1;
	if((text_was[row][col] == h)&&(text_was_n[row][col] == cells)) return 1;
	for(c = 0; c < 30; c++) if(text_was[row][c]&&(c < col+cells)&&(c+text_was_n[row][c] > col)) text_was[row][c] = 0;
	text_was[row][col] = h;
	text_was_n[row][col] = cells;

	return 0;
}

// rows y to y+h-1 drawn over another way
static void text_forget(int y, int h){

	int 	r;

	for(r = (y < 0) ? 0 : (y >> 3); (r <= ((y+h-1) >> 3))&&(r < 20); r++) memset(text_was[r], 0, sizeof(text_was[r]));
}
static int cursor_x = -1, cursor_y;		// gba_cursor()'s

// 8 4bpp pixels of g at x, y on the text layer where m's are set, x >= 0
static void text_put(int x, int y, u32 g, u32 m){

	int 	tx = x >> 3, s = 4*(x&7);
	u32 	* t;

	if((y < 0)||(y >= SCREEN_H)||(tx >= 30)) return;
	t = MENU_TEXT+8*(30*(y >> 3)+tx)+(y&7);
	*t = (*t&~(m << s))|(g << s);
	if(s&&(tx < 29)) t[8] = (t[8]&~(m >> (32-s)))|(g >> (32-s));
}

// the same, see-through where g is 0, and in gba_text_color()'s shade
static void text_glyph_row(int x, int y, u32 g){

	u32 	m;

	if(!g) return;
	if(text_shade < 15){

		u32 	o = 0;
		int 	k;

		for(k = 0; k < 32; k += 4){

			u32 	v = (g >> k)&15;

			if(v) o |= ((v*text_shade+14)/15) << k;
		}
		g = o;
	}
	m = g|(g >> 1)|(g >> 2)|(g >> 3);
	m = (m&0x11111111)*15;
	text_put(x, y, g, m);
}

// cells col, row on, n of them, in gba_text_color()'s ramp
static void text_bank_cells(int col, int row, int n){

	for(; n > 0; n--, col++) if((col >= 0)&&(col < 30)&&(row >= 0)&&(row < 20)) MENU_TEXT_MAP[32*row+col] = (MENU_TEXT_TILE0+30*row+col)|(text_bank << 12);
}

// the text from now on: menu.pal's ramp from color 16*bank, 0 the grey,
// 2 the blue and 3 the fire's, at its brightest shade, 15 the most
void gba_text_color(int bank, int shade){

	text_bank = bank;
	text_shade = shade;
}

// pixel rows y to y+h-1 of the text layer cleared
void gba_text_clear(int y, int h){

	int 	tx;

	text_forget(y, h);
	for(; h > 0; h--, y++) for(tx = 0; tx < 30; tx++) text_put(8*tx, y, 0, 0xffffffff);
}

// the menus' background, and no text on it
void gba_text_screen(void){

	int 	n;

	view_drawing = 0;
	VBlankIntrWait();
	REG_DISPCNT = LCDC_OFF;
	for(n = 0; n < 128; n++) OAM[n].attr0 = OBJ_DISABLE;
	shown_model = -1;
	shown_part = 0;
	cursor_x = -1;
	REG_BG1CNT = BG_16_COLOR | CHAR_BASE(2) | SCREEN_BASE(20) | BG_SIZE_0 | BG_PRIORITY(1);
	REG_BG2CNT = BG_16_COLOR | CHAR_BASE(3) | SCREEN_BASE(21) | BG_SIZE_0 | BG_PRIORITY(2);
	REG_BG3CNT = BG_256_COLOR | CHAR_BASE(0) | SCREEN_BASE(19) | BG_SIZE_0 | BG_PRIORITY(3);
	REG_BG1HOFS = REG_BG1VOFS = REG_BG2HOFS = REG_BG2VOFS = REG_BG3HOFS = REG_BG3VOFS = 0;
	copy32(BG_PALETTE, menu_pal, 512);
	// the boxes' colors: 0xc4, then the grey ramp, in a bank nothing else uses
	BG_PALETTE[16*MENU_BOX_BANK+1] = ((const u16 *)menu_pal)[0xc4];
	for(n = 2; n < 16; n++) BG_PALETTE[16*MENU_BOX_BANK+n] = ((const u16 *)menu_pal)[n];
	copy32((u32 *)CHAR_BASE_BLOCK(3)+8*MENU_BOX_TILE0, box_bin, 22*32);
	for(n = 0; n < 32*32; n++) MENU_BOX_MAP[n] = MENU_BOX_TILE0;
	// gbabake's menubg.dtl: the map, then the tiles
	copy32((void *)CHAR_BASE_BLOCK(0), menubg_dtl+30*20, 0x9800);
	for(n = 0; n < 32*32; n++) MENU_BG_MAP[n] = ((n&31) < 30)&&(n < 32*20) ? menubg_dtl[30*(n >> 5)+(n&31)] : 0;
	for(n = 0; n < 8*(30*20+1); n++) MENU_TEXT[n] = 0;
	for(n = 0; n < 32*32; n++) MENU_TEXT_MAP[n] = MENU_BLANK_TILE;
	gba_text_color(0, 15);
	for(n = 0; n < 20; n++) text_bank_cells(0, n, 30);
	text_forget(0, SCREEN_H);
	// the track's colors for the sprites again, which gba_picture() changes
	copy32(SPRITE_PALETTE, reversed ? trk->rev_pal : trk->pal, 512);
	REG_DISPCNT = MENU_DISPCNT;
}

// one of the menus' 64x64 pictures (gbabake's pictures.obj and faces.obj)
// on the text screen in sprite slot 0, 1 or 2, its top left at x, y, in the
// menus' colors: not with gba_show_car()'s car, whose colors are the
// track's. obj 0 hides it
void gba_picture(int slot, const u8 * obj, int x, int y){

	extern const u8 menu_pal[];

	OBJATTR 	* o = &OAM[1+slot];

	if(!obj){

		o->attr0 = OBJ_DISABLE;
		return;
	}
	copy32(SPRITE_PALETTE, menu_pal, 512);
	copy32((u8 *)SPRITE_GFX+(8+4*slot)*1024, obj, 64*64);
	REG_DISPCNT = MENU_DISPCNT;
	o->attr0 = OBJ_Y(y) | OBJ_256_COLOR | OBJ_SQUARE;
	o->attr1 = OBJ_X(x) | OBJ_SIZE(3);
	o->attr2 = OBJ_CHAR(256+128*slot) | OBJ_PRIORITY(0);
}

// The side panel with your car and its upgrades (the desktop's
// ___25330h()), 48x112, its top left at x, y, in sprites 4 and 5 from
// OBJ VRAM's 20 KB on, in the menus' colors. gba_panel_*() draw on it,
// in its pixels
#define PANEL_GFX		((u8 *)SPRITE_GFX+20*1024)

void gba_panel(int x, int y){

	int 	k;

	copy32(PANEL_GFX, panel_bin, 2*64*64);
	for(k = 0; k < 2; k++){

		OAM[4+k].attr0 = OBJ_Y(y+64*k) | OBJ_256_COLOR | OBJ_SQUARE;
		OAM[4+k].attr1 = OBJ_X(x) | OBJ_SIZE(3);
		OAM[4+k].attr2 = OBJ_CHAR(640+128*k) | OBJ_PRIORITY(0);
	}
	REG_DISPCNT = MENU_DISPCNT;
}

static void panel_px(int x, int y, int c){

	int 		o;
	vu16 		* p;

	if((x < 0)||(x >= 48)||(y < 0)||(y >= 112)) return;
	o = 4096*(y >> 6)+64*(8*((y&63) >> 3)+(x >> 3))+8*(y&7)+(x&7);
	p = (vu16 *)(PANEL_GFX+(o&~1));
	*p = (o&1) ? ((*p&0xff)|(c << 8)) : ((*p&0xff00)|c);
}

// a w x h picture at x, y on it, color 0 see-through
void gba_panel_image(const u8 * px, int w, int h, int x, int y){

	int 	i, j;

	for(j = 0; j < h; j++) for(i = 0; i < w; i++) if(px[w*j+i]) panel_px(x+i, y+j, px[w*j+i]);
}

void gba_panel_fill(int x, int y, int w, int h, int c){

	int 	i, j;

	for(j = 0; j < h; j++) for(i = 0; i < w; i++) panel_px(x+i, y+j, c);
}

// s in the small font, its top left at x, y, in gba_text_color()'s
void gba_panel_text(int x, int y, const char * s){

	int 	r, k;

	for(; *s; s++){

		int 	c = (*s&127)-' ';

		if(c < 0) c = 0;
		if(c) for(r = 0; r < 8; r++){

			u32 	g = FONT_SMALL[8*c+r];

			for(k = 0; k < 8; k++, g >>= 4) if(g&15) panel_px(x+k, y+r, 16*text_bank+((g&15)*text_shade+14)/15);
		}
		x += FONT_WIDTHS[c];
	}
}

// your car's body in the menus' colors 64-95, as ___11378h_cdecl() makes
// them from a 6-bit VGA color
void gba_panel_car_color(int r, int g, int b){

	int 	n;

	for(n = 0; n < 16; n++){

		int 	d[3] = { r*(9*n+16)/160, g*(9*n+16)/160, b*(9*n+16)/160 };
		int 	l[3] = { r+n*(63-r)/16, g+n*(63-g)/16, b+n*(63-b)/16 };

		SPRITE_PALETTE[0x40+n] = (d[0] >> 1)|((d[1] >> 1) << 5)|((d[2] >> 1) << 10);
		SPRITE_PALETTE[0x50+n] = (l[0] >> 1)|((l[1] >> 1) << 5)|((l[2] >> 1) << 10);
	}
}

// a car turning on the text screen, its top left at x, y; model -1 hides
// it. gba_turn_car() shows its next frame, once a frame

void gba_show_car(int model, int x, int y){

	int 	n;

	// the car's colors and its OBJ's empty tiles, after a part's; -1
	// hides either
	if(shown_part&&(model >= 0)){

		u32 	* t = (u32 *)SPRITE_GFX;

		copy32(SPRITE_PALETTE, reversed ? trk->rev_pal : trk->pal, 512);
		for(n = 0; n < 4*8*16; n++) t[n] = 0;
	}
	shown_part = 0;
	shown_model = model;
	if(model < 0){

		OAM[0].attr0 = OBJ_DISABLE;
		return;
	}
	REG_DISPCNT = MENU_DISPCNT;
	OAM[0].attr0 = OBJ_Y(y) | OBJ_256_COLOR | OBJ_SQUARE;
	OAM[0].attr1 = OBJ_X(x) | OBJ_SIZE(3);
	OAM[0].attr2 = OBJ_CHAR(0) | OBJ_PRIORITY(0);
	gba_turn_car();
}

// the shown car's 10 body colors (car 0's, from 15 on), as ___4a064h_cdecl()
// makes them from a 6-bit VGA color
void gba_car_color(int r, int g, int b){

	int 	i, k, c[3] = { r, g, b }, v[2][3];

	for(i = 0; i < 5; i++){

		for(k = 0; k < 3; k++){

			v[0][k] = (int)((0.18*i+0.1)*c[k]);
			v[1][k] = (int)(12.6*i+(1.0-0.2*i)*c[k]);
		}
		SPRITE_PALETTE[15+i] = (v[0][0] >> 1)|((v[0][1] >> 1) << 5)|((v[0][2] >> 1) << 10);
		SPRITE_PALETTE[20+i] = (v[1][0] >> 1)|((v[1][1] >> 1) << 5)|((v[1][2] >> 1) << 10);
	}
}

// one of the shop's animations (gbabake's shop.lz) in the car's place,
// n frames of 48x32 from frames on, or none for 0; bounce plays them
// forward and back, as the armor's go. gba_turn_car() moves it on
void gba_show_part(const u32 * frames, int n, int bounce, int x, int y){

	extern const u8 menu_pal[];

	if(!frames){

		if(shown_part&&(shown_model < 0)) OAM[0].attr0 = OBJ_DISABLE;
		shown_part = 0;
		return;
	}
	if(!shown_part) copy32(SPRITE_PALETTE, menu_pal, 512);
	shown_model = -1;
	shown_part = frames;
	shown_part_frame = -1;
	part_frames = n;
	part_bounce = bounce;
	REG_DISPCNT = MENU_DISPCNT;
	OAM[0].attr0 = OBJ_Y(y) | OBJ_256_COLOR | OBJ_WIDE;
	OAM[0].attr1 = OBJ_X(x) | OBJ_SIZE(3);		// 64x32
	OAM[0].attr2 = OBJ_CHAR(0) | OBJ_PRIORITY(0);
	gba_turn_car();
}

void gba_turn_car(void){

	const u8 * 	frames;

	if(shown_part){

		// about the 35 frames a second the desktop's shop shows them at
		int 	f = (vblanks/2)%(part_bounce ? 2*part_frames-2 : part_frames);

		if(f >= part_frames) f = 2*part_frames-2-f;
		// the OBJ's tiles, from the BIOS, in 16 bits for VRAM
		if(f != shown_part_frame) LZ77UnCompVram((const u8 *)shop_lz+shown_part[f], SPRITE_GFX);
		shown_part_frame = f;
		return;
	}
	if(shown_model < 0) return;
	frames = cars_img+shown_model*CAR_FRAMES*CAR_FRAME_BYTES+((vblanks/3)%CAR_FRAMES)*CAR_FRAME_BYTES;
	tile_car((u32 *)SPRITE_GFX, (const u32 *)frames, 0);
}

// s in the small font, in the 8x8 cells from col, row on, which it clears
// first, as many as it has characters. A word after two spaces or more
// starts in its own cell, so the screens' columns, which they line up with
// spaces, still line up
void gba_print(int col, int row, const char * s){ gba_print_cells(col, row, 30, s); }

// the same in no more than cells cells: what doesn't fit isn't drawn
void gba_print_cells(int col, int row, int cells, const char * s){

	int 	n = strlen(s), x = 8*col, k, y, end;

	if(col < 0){ s -= col; n += col; x = col = 0; }
	if(cells > n) cells = n;
	if(col+cells > 30) cells = 30-col;
	if((cells <= 0)||text_known(col, row, cells, s)) return;
	end = 8*(col+cells);
	for(y = 8*row; y < 8*row+8; y++) for(k = 0; k < cells; k++) text_put(8*(col+k), y, 0, 0xffffffff);
	text_bank_cells(col, row, cells);
	for(k = 0; k < n; k++){

		int 	c = (s[k]&127)-' ';

		if(c < 0) c = 0;
		if((k == 0)||((k >= 2)&&(s[k-1] == ' ')&&(s[k-2] == ' '))) if(x < 8*(col+k)) x = 8*(col+k);
		if(x+FONT_WIDTHS[c] > end) break;
		if(c) for(y = 0; y < 8; y++) text_glyph_row(x, 8*row+y, FONT_SMALL[8*c+y]);
		x += FONT_WIDTHS[c];
	}
}

// how wide the first n characters of s are in the small font, in pixels
int gba_text_width(const char * s, int n){

	int 	w = 0;

	for(; *s&&(n > 0); s++, n--) w += FONT_WIDTHS[((*s&127) < ' ') ? 0 : (*s&127)-' '];

	return w;
}

// s in the small font in the middle of row, which it clears
void gba_center(int row, const char * s){

	int 	x = 0, k, y;

	for(k = 0; s[k]; k++) x += FONT_WIDTHS[((s[k]&127) < ' ') ? 0 : (s[k]&127)-' '];
	x = (SCREEN_W-x)/2;
	if(x < 0) x = 0;
	if(text_known(0, row, 30, s)) return;
	for(y = 8*row; y < 8*row+8; y++) for(k = 0; k < 30; k++) text_put(8*k, y, 0, 0xffffffff);
	text_bank_cells(0, row, 30);
	for(; *s; s++){

		int 	c = (*s&127)-' ';

		if(c < 0) c = 0;
		if(c) for(y = 0; y < 8; y++) text_glyph_row(x, 8*row+y, FONT_SMALL[8*c+y]);
		x += FONT_WIDTHS[c];
	}
}

// how wide s is in the big font, in pixels
int gba_big_width(const char * s){

	int 	w = 0;

	for(; *s; s++) w += FONT_WIDTHS[96+(((*s&127) < ' ') ? 0 : (*s&127)-' ')];

	return w;
}

// s in the big font, 16 pixels high, its top left at x, y, over what's
// there: gba_text_clear() clears it
void gba_print_big(int x, int y, const char * s){

	int 	r;

	text_forget(y, 16);
	for(r = (y >> 3); r <= ((y+15) >> 3); r++) text_bank_cells(x >> 3, r, ((x+gba_big_width(s)) >> 3)-(x >> 3)+1);
	for(; *s&&(x < SCREEN_W); s++){

		int 	c = (*s&127)-' ';

		if(c < 0) c = 0;
		if(c) for(r = 0; r < 16; r++){

			text_glyph_row(x, y+r, FONT_BIG[32*c+2*r]);
			text_glyph_row(x+8, y+r, FONT_BIG[32*c+2*r+1]);
		}
		x += FONT_WIDTHS[96+c];
	}
}

// the desktop's box (___13248h_cdecl()), w x h cells from col, row on, at
// least 4 x 4, under the text: its corners, edges and inside
void gba_box(int col, int row, int w, int h){

	int 	x, y;

	for(y = 0; y < h; y++) for(x = 0; x < w; x++){

		int 	t, cx = (x < 2) ? x : (x >= w-2) ? x-w+4 : -1, cy = (y < 2) ? y : (y >= h-2) ? y-h+4 : -1;

		if((cx >= 0)&&(cy >= 0)) t = 1+4*((cx >= 2)+2*(cy >= 2))+2*(cy&1)+(cx&1);
		else if(y == 0) t = 17;
		else if(y == h-1) t = 18;
		else if(x == 0) t = 19;
		else if(x == w-1) t = 20;
		else t = 21;
		if((col+x >= 0)&&(col+x < 32)&&(row+y >= 0)&&(row+y < 32)) MENU_BOX_MAP[32*(row+y)+col+x] = (MENU_BOX_TILE0+t)|(MENU_BOX_BANK << 12);
	}
}

// the desktop's spinning cursor, 10x10, its top left at x, y, or none for
// x -1. gba_frame() turns it
void gba_cursor(int x, int y){

	int 	r;

	// where it is already: drawn over, it'd be gone until it turns again
	if((x == cursor_x)&&((x < 0)||(y == cursor_y))) return;
	if(cursor_x >= 0) for(r = 0; r < 10; r++){

		text_put(cursor_x, cursor_y+r, 0, 0xffffffff);
		text_put(cursor_x+8, cursor_y+r, 0, 0xff);
	}
	cursor_x = x;
	cursor_y = y;
}

static void cursor_turn(void){

	const u32 	* f = cursor_bin+20*((vblanks/2)%50);
	int 		r, bank = text_bank;

	if(cursor_x < 0) return;
	text_bank = 0;
	text_bank_cells(cursor_x >> 3, cursor_y >> 3, 2);
	text_bank_cells(cursor_x >> 3, (cursor_y >> 3)+1, 2);
	text_bank = bank;
	for(r = 0; r < 10; r++){

		text_put(cursor_x, cursor_y+r, f[2*r], 0xffffffff);
		text_put(cursor_x+8, cursor_y+r, f[2*r+1], 0xff);
	}
}

// the next frame, and the keys pressed since the last, the arrows
// repeating while they're held
u16 gba_frame(void){

	VBlankIntrWait();
	cursor_turn();
	scanKeys();

	return keysDown()|(keysDownRepeat()&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT));
}

static void fail(const char * err);

// what the game's ___3f71ch__allocateMemory() calls when it's out of
// memory, before it prints so and exits: here it stops at fail()
void dRally_System_clean(void){ fail("out of memory"); }
void __VGA3_SETMODE(void){}

static void fail(const char * err){

	hud_print(0, 0, "ERR");
	say("%s", err);
	REG_DISPCNT = MODE_0 | BG1_ON;
	while(1) VBlankIntrWait();
}

// the race start_race() set up, until it's over and you press Start or A
// (never in the benchmark)
static void run_race(void){

	int 		n, frames = 0, over = 0;
	u32 		last_report = 0, last_drawn = 0, last_isr, t;
#if !defined(RACE_BENCH)
	u32 		last_vblank, ticks_due = 0;
#endif
	u32 		idle = 0, logic = 0, render = 0, ticks = 0, logic_max = 0;
	char 		line[32];


	VBlankIntrWait();	// which draws it
	REG_DISPCNT = MODE_0 | BG0_ON | BG1_ON | BG2_ON | OBJ_ON | OBJ_1D_MAP | OBJ_WIN_ON;
#if !defined(RACE_BENCH)
	last_vblank = vblanks;
#endif
	last_isr = isr_cycles;

	while(1){

		u16 	keys, down;
		int 	k;
		u32 	i;

#if defined(RACE_BENCH)
		// a tick a frame, however long frames take, so that the race is
		// the same whatever the code's speed
		t = cycles();
		VBlankIntrWait();
		idle += cycles()-t;
#else
		// wait only while no tick is due: a frame of the logic that took
		// too long goes on with the next straight away
		while(ticks_due+(vblanks - last_vblank)*TICKS_PER_SEC < VBLANKS_PER_SEC){

			t = cycles();
			VBlankIntrWait();
			idle += cycles()-t;
		}
#endif

		scanKeys();
		keys = keysHeld();
		down = keysDown();
		// once it's over, Start or A leaves; before, Start lets the AI
		// drive your car or not
		if(over&&(down & (KEY_START|KEY_A))) break;
#if defined(CAREER_BENCH)
		if(over) break;		// the career goes on by itself
#endif
		if(down & KEY_START) D(___243ce0h) = !D(___243ce0h);
		read_keys(keys, down);

		// the game's 70 Hz timer, for the vblanks since the last frame:
		// input into your car's controls, and the ticks race_logic() runs
#if defined(RACE_BENCH)
		k = 1;
		race___40164h();
		timer_ticks++;
#else
		ticks_due += (vblanks - last_vblank)*TICKS_PER_SEC;
		last_vblank = vblanks;
		if(ticks_due > MAX_DUE) ticks_due = MAX_DUE;
		ticks_due -= VBLANKS_PER_SEC;
		k = 1;
		race___40164h();
		timer_ticks++;
#endif

		if(!over){

			// the interrupt's drawing left out
			i = isr_cycles;
			t = cycles();
			race_logic();
			t = cycles()-t-(isr_cycles-i);
			logic += t; ticks += k;
			if(t > logic_max) logic_max = t;
#if defined(RACE_BENCH)
			bench_ticks++; bench_logic += t;
			if(t > bench_max) bench_max = t;
			if(t > 190000) bench_over190++;
			if(t > 200000) bench_over200++;
			if(t > 225000){ bench_over225++; say("BENCH slow tick %lu: %lu", bench_ticks, t); }
#else
			// the next tick is due already: the box can wait
			box_wait = ticks_due+(vblanks - last_vblank)*TICKS_PER_SEC >= VBLANKS_PER_SEC;
#endif

			i = isr_cycles;
			t = cycles();
			render_race();
			race_render_after();
			render += cycles()-t-(isr_cycles-i);
			if((int)D(___2438c8h) > RACE_OVER){

				over = 1;
#if defined(RACE_BENCH)
				race_replay_gather(flat);
				say("BENCH end: %lu ticks, logic %llu cycles a tick, at most %lu; over 190k %lu, 200k %lu, 225k %lu; globals hash %08lx",
					bench_ticks, bench_logic/bench_ticks, bench_max, bench_over190, bench_over200, bench_over225, race_replay_hash(flat));
				for(n = 0; n < NUM_OF_CARS; n++) say("BENCH car %d: lap %d pos %d fin %lu", n, car(n)->Lap, car(n)->Position, car(n)->Finished);
#if defined(VAR_HASHES)
				{
					unsigned 	i, o, k;
					__DWORD__ 	h;

					for(i = o = 0; i < race_replay_count(); o += race_replay_var_size(i++)){
						for(h = 0x811c9dc5, k = 0; k < race_replay_var_size(i); k++) h = (h^flat[o+k])*0x01000193;
						say("end var %s %08lx", race_replay_var_name(i), h);
					}
				}
#endif
				{
					// what the race drew into the track, as tools/replay --race prints it
					unsigned 	used, max, dropped;
					__DWORD__ 	h = 0x811c9dc5;
					int 		x, y;

					for(y = 0; y < TRX_HEIGHT; y++){

						race_ima_read(0, y, flat, TRX_WIDTH);
						for(x = 0; x < TRX_WIDTH; x++) h = (h^flat[x])*0x01000193;
					}
					race_ima_stats(&used, &max, &dropped);
					say("BENCH track image %08lx: %u of %u tiles drawn into, %u writes dropped", h, used, max, dropped);
				}
#endif
				// the places, and with them the damage and the kills,
				// as race_main() leaves them in ___1de7d0h
				for(n = 0; n < NUM_OF_CARS; n++) ((struct_94_t *)___1de580h)[n].__54 = car(n)->Position;
				race___400ech();
			}
		}
		follow_camera();
		take_view();
		frames++;

		if(vblanks - last_report >= VBLANKS_PER_SEC){

			char * 	p = line;

			// frames a second, and the CPU they took of the time there was
			// the frames shown, and the CPU used of the time there was
			u32 	shown = views_drawn - last_drawn, spent = (vblanks - last_report)*FRAME_CYCLES;

			memcpy(p, "FPS", 3); p = put_num(p + 3, shown*VBLANKS_PER_SEC/(vblanks - last_report), 3);
			memcpy(p, " CPU", 4); p = put_num(p + 4, idle < spent ? (unsigned long long)(spent-idle)*100/spent : 0, 3);
			*p++ = '%'; *p = 0;
			hud_print(SCREEN_W/8-14, 19, line);
			say("%s: %lu ticks, logic %lu cycles a tick, at most %lu a frame; render %lu, drawing %lu a frame",
				line, ticks, ticks ? logic/ticks : 0, logic_max, frames ? render/frames : 0,
				shown ? (isr_cycles-last_isr)/shown : 0);
#if defined(RACE_TRACE)
			say("me %lu ai %lu", MY_CAR_IDX, D(___243ce0h));
			for(n = 0; n < NUM_OF_CARS; n++){

				say("car %d: %d,%d dir %d lap %d pos %d fin %lu", n, (int)car(n)->XLocation, (int)car(n)->YLocation,
					(int)car(n)->Direction, car(n)->Lap, car(n)->Position, car(n)->Finished);
			}
#endif
			if(&dr_misaligned&&dr_misaligned) say("ALIGNED_CHECK: %u unaligned, the first at %s:%u", dr_misaligned, dr_misaligned_file, dr_misaligned_line);
			last_report = vblanks;
			last_drawn = views_drawn;
			last_isr = isr_cycles;
			frames = 0; idle = logic = render = ticks = logic_max = 0;
		}
	}
	view_drawing = 0;
}

// quick races, one after another: the track and the rest chosen before
// each, as long as there's a choice. Until B in the choice
static void quick_races(void){

	const char * 	err;
	u32 		t;

	for(;;){

		if((race_tracks_n > 1)&&choose_race()) return;
		t = vblanks;
		if((err = start_race(0, 0, 0))) fail(err);
		say("race set up in %lu frames", vblanks-t);
		run_race();
	}
}

// a new career: the race's globals as at start-up, as a quick race
// before it may have left them, like the desktop game's when it starts
void gba_career_reset(void){

	race_replay_scatter(flat_boot);
}

// the career's race, which career.c set up
void gba_career_race(int my_index, int num_cars){

	const char * 	err;

	if((err = start_race(1, my_index, num_cars))) fail(err);
	run_race();
	sound_music(-1);
}

int main(void){

	int 		n;

	// the IWRAM below the stack, marked so that ojgbarun's stackfree= can
	// tell how much of it the stack ever reaches
	{
		extern u32 __iheap_start[];
		u32 * p = __iheap_start, * sp = __builtin_frame_address(0);

		for(; p < sp-64; p++) *p = 0x57AC57AC;
	}

	REG_WAITCNT = 0x4017;		// ROM at 3/1 wait states with prefetch, works on flash carts
	REG_DEBUG_ENABLE = 0xC0DE;
	debug_log = (REG_DEBUG_ENABLE == 0x1DEA);
	{
		// assets.S leaves the last two of sprites.bin out
		unsigned 	i, size = 0;

		for(i = 0; i < RACE_SPRITE_FILES; i++) size += race_sprite_files[i].size;
		if((size != 369024)||(race_sprite_files[14].size != 0x3840)||(race_sprite_files[15].size != 0x4b000)) say("sprites.bin isn't as assets.S has it");
	}

	sound_init();			// before the vblank handler, which mixes
	irqInit();
	irqSet(IRQ_VBLANK, vblank_handler);
	irqEnable(IRQ_VBLANK);
	REG_TM3CNT_H = TIMER_START | TIMER_COUNT;
	REG_TM2CNT_H = TIMER_START;

	for(n = 0; n < 256; n++) ring_row[n] = n % RING_H;

	REG_DISPCNT = LCDC_OFF;
	for(n = 0; n < 128; n++) OAM[n].attr0 = OBJ_DISABLE;
	use_track(&race_tracks[0]);

	// shadows: inside the OBJ window the track (BG0) is darkened
	REG_WINOUT = (WIN_BG0 | WIN_BG1 | WIN_BG2 | WIN_OBJ) | ((WIN_BG0 | WIN_BG1 | WIN_BG2 | WIN_OBJ | WIN_BLD) << 8);
	REG_BLDCNT = BLEND_BG0 | BLEND_DARKEN | BLEND_BG0_UNDER;
	REG_BLDY = 9;

	race_replay_gather(flat_boot);
	setRepeat(20, 5);
#if defined(CAREER_BENCH)
	(void)quick_races;
	career_play();
#elif defined(RACE_BENCH)||defined(RACE_PRE)
	quick_races();
#else
	// a ROM with one track has only its quick race
	if(race_tracks_n == 1) for(;;) quick_races();
	for(;;){

		if(gba_title() == 0) career_play();
		else quick_races();
	}
#endif
}
