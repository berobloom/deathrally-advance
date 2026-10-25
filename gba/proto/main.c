// Death Rally GBA prototype: drive a car around a real track.
//
// Tests whether the GBA's hardware can draw a race: the track is a tile
// background streamed from ROM, cars are 64x64 sprites showing the game's
// own 96 rotation frames, and their shadows darken the track through the OBJ
// window. The race logic is a stand-in; none of the game's code runs here.
//
// D-pad left/right steer, A accelerates, B brakes and reverses,
// Start goes back to the start line, Select hides the frame meter, R lets the
// autopilot that drives the other cars drive yours too.

#include <gba_base.h>
#include <gba_video.h>
#include <gba_sprites.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_dma.h>
#include <string.h>

// baked by tools/gbabake, see assets.S
extern const u16 track_pal[256];
extern const u8 track_tiles[];
extern const u8 track_mas[];
extern const u32 track_inf[];
extern const u8 cars_obj[];

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
#define CAR_OBJ_BYTES		(64*64)		// OBJ VRAM for each car's 64x64 sprite
#define CAR_TILES		5		// its 40x40 frame, 5x5 tiles from the top left
#define CAR_FRAME_BYTES		(CAR_TILES*CAR_TILES*64)	// a frame in cars.obj
#define NUM_CARS		4
#define MAS_ROAD		15		// track mask value of drivable road

// the track background: a ring of 32x22 tiles in charblocks 0-2, map in screenblock 31
#define RING_W			32
#define RING_H			22
#define TRACK_MAP		((u16 *)SCREEN_BASE_BLOCK(31))
#define TRACK_CHARS		((u8 *)CHAR_BASE_BLOCK(0))
// the frame meter: 8bpp glyphs in charblock 3, map in screenblock 30
#define HUD_MAP			((u16 *)SCREEN_BASE_BLOCK(30))
#define HUD_CHARS		((u8 *)CHAR_BASE_BLOCK(3))

typedef struct {
	s32	x, y;		// position of the car's center, 24.8 pixels
	s32	vx, vy;		// velocity, 24.8 pixels per frame
	s32	speed;		// along the heading, 24.8 pixels per frame
	s32	angle;		// heading, 1/16 rotation frames, counterclockwise from north
	int	shown_frame;	// rotation frame currently in OBJ VRAM
} car_t;

static car_t cars[NUM_CARS];
static int track_w, track_h;		// in tiles
static int track_pw, track_ph;		// in pixels; the tiles are padded to a multiple of 8
static int cam_x, cam_y;
static int vis_x0 = -1, vis_y0, vis_x1, vis_y1;
static volatile u32 vblanks;
static u8 ring_row[256];		// ty % RING_H, without dividing
static int debug_log;

// sin(n * 360/96 degrees) * 4096
static const s16 sin96[CAR_FRAMES] = {
	0, 268, 535, 799, 1060, 1316, 1567, 1810, 2048, 2276, 2496, 2706,
	2896, 3076, 3244, 3400, 3547, 3675, 3784, 3876, 3952, 4011, 4053, 4078,
	4096, 4078, 4053, 4011, 3952, 3876, 3784, 3675, 3547, 3400, 3244, 3076,
	2896, 2706, 2496, 2276, 2048, 1810, 1567, 1316, 1060, 799, 535, 268,
	0, -268, -535, -799, -1060, -1316, -1567, -1810, -2048, -2276, -2496, -2706,
	-2896, -3076, -3244, -3400, -3547, -3675, -3784, -3876, -3952, -4011, -4053, -4078,
	-4096, -4078, -4053, -4011, -3952, -3876, -3784, -3675, -3547, -3400, -3244, -3076,
	-2896, -2706, -2496, -2276, -2048, -1810, -1567, -1316, -1060, -799, -535, -268
};

// unit vector a car moves along at rotation frame f, 4096 = 1 pixel. As in
// race___4c434h.c: Direction = 3.75 * frame degrees, forward is
// (-sin, -cos * 0.83), so frame 0 heads north and frame 24 west
static void heading(int frame, s32 * dx, s32 * dy){

	*dx = -sin96[frame];
	*dy = -(sin96[(frame + 24) % CAR_FRAMES] * 850 >> 10);	// cos * 0.83
}

static void copy32(void * dst, const void * src, u32 bytes){

	REG_DMA3SAD = (u32)src;
	REG_DMA3DAD = (u32)dst;
	REG_DMA3CNT = DMA_ENABLE | DMA32 | (bytes >> 2);
}

static void debug_print(const char * s){

	if(!debug_log) return;
	strncpy(REG_DEBUG_STRING, s, 255);
	REG_DEBUG_FLAGS = 3 | 0x100;	// info level, print
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

// --- frame meter -----------------------------------------------------------

// 5x7 glyphs, one byte per row, bit 4 is the leftmost pixel
static const char hud_chars[] = "0123456789FPSCU%GX ";
static const u8 hud_font[][7] = {
	{14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31}, {31,2,4,2,1,17,14},
	{2,6,10,18,31,2,2}, {31,16,30,1,1,17,14}, {6,8,16,30,17,17,14}, {31,1,2,4,8,8,8},
	{14,17,17,14,17,17,14}, {14,17,17,15,1,2,12}, {31,16,16,30,16,16,16}, {30,17,17,30,16,16,16},
	{15,16,16,14,1,1,30}, {14,17,16,16,16,17,14}, {17,17,17,17,17,17,14}, {24,25,2,4,8,19,3},
	{14,17,16,23,17,17,15}, {17,17,10,4,10,17,17}, {0,0,0,0,0,0,0}
};

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

	for(x = 0; x < 32*32; x++) HUD_MAP[x] = sizeof(hud_chars)-2;	// space
}

static void hud_print(int col, int row, const char * s){

	for(; *s; s++, col++) HUD_MAP[row*32 + col] = strchr(hud_chars, *s) - hud_chars;
}

static char * put_num(char * p, int v, int digits){

	int n;

	for(n = digits-1; n >= 0; n--, v /= 10) p[n] = (n < digits-1)&&(v == 0) ? ' ' : '0' + v%10;
	return p + digits;
}

// --- cars --------------------------------------------------------------------

static int mas_at(int px, int py){

	if((px < 0)||(py < 0)||(px >= track_pw)||(py >= track_ph)) return 0;
	return track_mas[py*track_pw + px];
}

// a car's body fits in about 14x14 pixels, whatever its heading
static int blocked(s32 x, s32 y){

	int px = x >> 8, py = y >> 8;

	return (mas_at(px-6, py-6) != MAS_ROAD)||(mas_at(px+6, py-6) != MAS_ROAD)||
		(mas_at(px-6, py+6) != MAS_ROAD)||(mas_at(px+6, py+6) != MAS_ROAD);
}

static void reset_cars(void){

	int n;

	for(n = 0; n < NUM_CARS; n++){

		const u32 * st = &track_inf[4 + 3*n];
		memset(&cars[n], 0, sizeof(car_t));
		cars[n].x = st[0] << 8;
		cars[n].y = st[1] << 8;
		cars[n].angle = (st[2] % CAR_FRAMES) << 4;
		cars[n].shown_frame = -1;
	}
}

static void drive(car_t * c, u16 keys){

	int frame, turn;
	s32 hx, hy, nx, ny;

	if(keys & KEY_A) c->speed += 7;
	else if(c->speed > 0) c->speed -= (c->speed >> 6) + 1;
	if(keys & KEY_B) c->speed -= (c->speed > 0) ? 16 : 5;
	if(!(keys & (KEY_A|KEY_B)) && (c->speed < 0)) c->speed += 2;
	if(c->speed > 5*256) c->speed = 5*256;
	if(c->speed < -384) c->speed = -384;

	// steer faster at speed, and the other way round when reversing
	turn = 4 + ((c->speed < 0 ? -c->speed : c->speed) * 12 >> 10);
	if(c->speed < 0) turn = -turn;
	if(keys & KEY_LEFT) c->angle += turn;
	if(keys & KEY_RIGHT) c->angle -= turn;
	c->angle = (c->angle + 16*CAR_FRAMES) % (16*CAR_FRAMES);

	// the velocity follows the heading with a little slide
	frame = c->angle >> 4;
	heading(frame, &hx, &hy);
	hx = (hx * c->speed) >> 12;
	hy = (hy * c->speed) >> 12;
	c->vx += (hx - c->vx) >> 3;
	c->vy += (hy - c->vy) >> 3;

	nx = c->x + c->vx;
	ny = c->y + c->vy;
	// against a wall, slide along it: only the part of the velocity into the wall bounces back
	if(!blocked(nx, ny)){ c->x = nx; c->y = ny; }
	else if(!blocked(nx, c->y)){ c->x = nx; c->vy = -c->vy >> 2; c->speed -= c->speed >> 5; }
	else if(!blocked(c->x, ny)){ c->y = ny; c->vx = -c->vx >> 2; c->speed -= c->speed >> 5; }
	else { c->vx = 0; c->vy = 0; c->speed = -c->speed >> 2; }
}

// distance to the edge of the road, looking dframes rotation frames off the heading
static int free_road(const car_t * c, int dframes){

	int frame = ((c->angle >> 4) + dframes + CAR_FRAMES) % CAR_FRAMES;
	s32 dx, dy;
	int d;

	heading(frame, &dx, &dy);
	for(d = 8; d < 96; d += 8){

		if(mas_at((c->x >> 8) + (dx*d >> 12), (c->y >> 8) + (dy*d >> 12)) != MAS_ROAD) break;
	}
	return d;
}

// stand-in for the race AI: steer toward the most open road
static u16 autopilot(const car_t * c, int top_speed){

	int left = free_road(c, 12), ahead = free_road(c, 0), right = free_road(c, -12);
	u16 keys = (c->speed < top_speed) ? KEY_A : 0;

	if((ahead < 40)&&(c->speed > 2*256)) keys = KEY_B;
	if((ahead < 64)||(left - right > 16)||(right - left > 16)) keys |= (left > right) ? KEY_LEFT : KEY_RIGHT;
	return keys;
}

static int car_on_screen(int n){

	int sx = (cars[n].x >> 8) - cam_x, sy = (cars[n].y >> 8) - cam_y;

	return (sx > -32)&&(sx < SCREEN_W+32)&&(sy > -32)&&(sy < SCREEN_H+32);
}

// copy a car's rotation frame into its OBJ VRAM if it changed and is in view
static void upload_car(int n){

	int frame = cars[n].angle >> 4, r;

	if((frame == cars[n].shown_frame)||!car_on_screen(n)) return;
	// a row of tiles at a time: the sprite's rows are 8 tiles wide
	for(r = 0; r < CAR_TILES; r++){

		copy32((u8 *)SPRITE_GFX + n*CAR_OBJ_BYTES + r*8*64, cars_obj + (n*CAR_FRAMES + frame)*CAR_FRAME_BYTES + r*CAR_TILES*64, CAR_TILES*64);
	}
	cars[n].shown_frame = frame;
}

static void place_cars(void){

	int n;

	for(n = 0; n < NUM_CARS; n++){

		int sx = (cars[n].x >> 8) - cam_x - 20, sy = (cars[n].y >> 8) - cam_y - 20;
		int tile = n*CAR_OBJ_BYTES/32;
		int hidden = !car_on_screen(n);

		// the car, then its shadow as OBJ window 5 pixels down and to the right
		OAM[2*n].attr0 = hidden ? OBJ_DISABLE : (OBJ_Y(sy) | OBJ_256_COLOR | OBJ_SQUARE);
		OAM[2*n].attr1 = OBJ_X(sx) | OBJ_SIZE(3);
		OAM[2*n].attr2 = OBJ_CHAR(tile) | OBJ_PRIORITY(1);
		OAM[2*n+1].attr0 = hidden ? OBJ_DISABLE : (OBJ_Y(sy+5) | OBJ_256_COLOR | OBJ_SQUARE | OBJ_MODE(2));
		OAM[2*n+1].attr1 = OBJ_X(sx+5) | OBJ_SIZE(3);
		OAM[2*n+1].attr2 = OBJ_CHAR(tile) | OBJ_PRIORITY(1);
	}
}

// --- main loop -----------------------------------------------------------------

static void follow_camera(void){

	// look ahead along the velocity, so more of the road ahead is in view
	int tx = (cars[0].x >> 8) + (cars[0].vx >> 4) - SCREEN_W/2;
	int ty = (cars[0].y >> 8) + (cars[0].vy >> 4) - SCREEN_H/2;

	cam_x += (tx - cam_x) / 4;
	cam_y += (ty - cam_y) / 4;
	if(cam_x < 0) cam_x = 0;
	if(cam_y < 0) cam_y = 0;
	if(cam_x > track_pw - SCREEN_W) cam_x = track_pw - SCREEN_W;
	if(cam_y > track_ph - SCREEN_H) cam_y = track_ph - SCREEN_H;
}

int main(void){

	int n, show_hud = 1, pilot = 0, frames = 0, lines_sum = 0, gfx_sum = 0;
	u32 last_report = 0;
	char line[32];

	REG_WAITCNT = 0x4007;		// ROM at 3/2 wait states with prefetch, works on flash carts
	REG_DEBUG_ENABLE = 0xC0DE;
	debug_log = (REG_DEBUG_ENABLE == 0x1DEA);

	irqInit();
	irqSet(IRQ_VBLANK, vblank_handler);
	irqEnable(IRQ_VBLANK);

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

	reset_cars();
	cam_x = (cars[0].x >> 8) - SCREEN_W/2;
	cam_y = (cars[0].y >> 8) - SCREEN_H/2;
	follow_camera();
	update_track();
	for(n = 0; n < NUM_CARS; n++) upload_car(n);
	place_cars();
	VBlankIntrWait();
	REG_DISPCNT = MODE_0 | BG0_ON | BG1_ON | OBJ_ON | OBJ_1D_MAP | OBJ_WIN_ON;

	while(1){

		u16 keys, down;
		int lines;

		VBlankIntrWait();

		// VRAM work first, while the screen isn't being drawn
		update_track();
		for(n = 0; n < NUM_CARS; n++) upload_car(n);
		place_cars();
		// the page bit has no effect in mode 0; toggling it lets ojgbarun's fps= count frames
		REG_DISPCNT ^= BACKBUFFER;
		gfx_sum += (REG_VCOUNT + 228 - 160) % 228;

		scanKeys();
		keys = keysHeld();
		down = keysDown();
		if(down & KEY_START){ reset_cars(); vis_x0 = -1; }
		if(down & KEY_SELECT){ show_hud = !show_hud; REG_DISPCNT ^= BG1_ON; }
		if(down & KEY_R) pilot = !pilot;

		drive(&cars[0], pilot ? autopilot(&cars[0], 4*256) : keys);
		for(n = 1; n < NUM_CARS; n++) drive(&cars[n], autopilot(&cars[n], (3*256) + 48*n));
		follow_camera();

		// time used this frame, in scanlines from the start of vblank
		lines = (REG_VCOUNT + 228 - 160) % 228;
		lines_sum += lines;
		frames++;

		if(vblanks - last_report >= 60){

			int fps = frames * 60 / (vblanks - last_report);
			char * p = line;

			memcpy(p, "FPS", 3); p = put_num(p + 3, fps, 3);
			memcpy(p, " CPU", 4); p = put_num(p + 4, lines_sum * 100 / (228 * frames), 3);
			*p++ = '%';
			memcpy(p, " GFX", 4); p = put_num(p + 4, gfx_sum * 100 / (228 * frames), 3);
			*p++ = '%'; *p = 0;
			if(show_hud) hud_print(0, 0, line);
			debug_print(line);
#ifdef PROTO_TRACE
			{
				char t[64]; char * q = t;
				q = put_num(q, cars[0].x >> 8, 5); *q++ = ' '; q = put_num(q, cars[0].y >> 8, 5); *q++ = ' ';
				q = put_num(q, cars[0].angle >> 4, 3); *q++ = ' '; q = put_num(q, cars[0].speed + 1000, 5); *q = 0;
				debug_print(t);
			}
#endif

			last_report = vblanks; frames = 0; lines_sum = 0; gfx_sum = 0;
		}
	}
}
