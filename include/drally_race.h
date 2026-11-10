#ifndef __DRALLY_RACE_H
#define __DRALLY_RACE_H

#include "drmath.h"

#define RACE_NOTWIDE    1
#define RACE_MSX        2
#define RACE_SFX        3
#define RACE_TEXTURES   4
#define RACE_SHADOWS    5

int dRally_Race_getSettings(int);
void dRally_Race_setSettings(int);
void dRally_Race_unsetSettings(int);
void dRally_Race_toggleSettings(int);

void race_msg(int msg_no);
void race_alloc(void);
void race_free(void);
// a build that never draws the race the game's way (the GBA's) defines
// DR_RACE_NODRAW, which leaves the drawing code out
#if defined(DR_RACE_NODRAW)
#define race_draw 0
#else
extern int race_draw;
#endif
void race_render(void);
void race_render_after(void);
// the race's pictures drawn over the track, in the order race_render()
// draws them: a w x h image's non-zero pixels with its top left at (bx, by)
// in BACKBUFFER. race_sprite() hands them to race_sprite_cb, if set, at
// (x, y) on the track, also in a build that doesn't draw (DR_RACE_NODRAW).
// img 0 marks where the cars go
extern void (*race_sprite_cb)(const __BYTE__ * img, int w, int h, int x, int y);
void race_sprite(const __BYTE__ * img, int w, int h, int bx, int by);
// the files of those images that the race reads with race___42824h(),
// decoded one after the other into tools/gbabake's sprites.bin
typedef struct {
	const char * 	name;		// in ENGINE.BPA
	unsigned 	size;
} race_sprite_file_t;
#define RACE_SPRITE_FILES	15
extern const race_sprite_file_t race_sprite_files[RACE_SPRITE_FILES];
void race_walls_build(void);
void race_walls_free(void);
int race_walls_near(int x, int y, int w, int h);
unsigned race_walls_cells(int x, int y, int w, int h);
// the box around each of a car's 96 images' solid pixels (above 3): first
// and last column, first and last row, or a first column of 40 if there
// are none. And each of their rows' solid pixels: the first and the last
// column, with bit 7 of the first set when all of them in between are
// solid too, or a first column of 40 if there are none
typedef struct {
	__BYTE__ 	box[96][4];
	__BYTE__ 	rows[96][0x28][2];
} race_car_shapes_t;
void race_car_shapes(const __BYTE__ * spins, race_car_shapes_t * out);
// car n's, which race_walls_build() builds where the loader set none
extern const race_car_shapes_t * race_car_shape[4];

// the track image, TRX_IMA, which the race code draws skid marks, mines,
// craters, power-ups and obstacles into: n pixels from (x, y) read, written,
// or the non-zero ones of src drawn, and a w x h block's pixels p turned
// into tab[p]. Pixel (x, y) is TRX_IMA[TRX_WIDTH*y+x], so a row runs on into
// the next one. Built with DR_IMA_TILES (the GBA), where the image doesn't
// fit in memory, it is the track's tiles with the ones drawn into copied
// to a pool of max_tiles, see race_ima.c
void race_ima_read(int x, int y, __BYTE__ * dst, int n);
void race_ima_write(int x, int y, const __BYTE__ * src, int n);
void race_ima_draw(int x, int y, const __BYTE__ * src, int n);
void race_ima_lookup(int x, int y, int w, int h, const __BYTE__ * tab);
#if defined(DR_IMA_TILES)
void race_ima_tiles(const __BYTE__ * tiles, int tw, int th, unsigned max_tiles);
const __BYTE__ * race_ima_tile(int tx, int ty);
// the next tile drawn into since the last call, ty << 8 | tx, -1 if there
// is none, or -2 for all of them
int race_ima_dirty(void);
void race_ima_stats(unsigned * used, unsigned * max, unsigned * dropped);
#endif

// car n's 96 rotation frames, 40x40 each, start at race_car_slot[n]. The
// desktop has the 4 one after the other at ___243d74h; the GBA points
// each into its ROM. A car's ImgOffset is 0x25800*n+0x640*frame
extern __BYTE__ * race_car_slot[4];
void race_car_slots_contiguous(void);
static inline __BYTE__ * race_car_img(__DWORD__ img_offset){

	int 	n = (img_offset >= 0x25800)+(img_offset >= 2*0x25800)+(img_offset >= 3*0x25800);

	return race_car_slot[n]+(img_offset-0x25800*n);
}

// the shapes of the image at img_offset, as race_car_img() finds it
static inline const race_car_shapes_t * race_car_shape_of(__DWORD__ img_offset, int * frame){

	int 	n = (img_offset >= 0x25800)+(img_offset >= 2*0x25800)+(img_offset >= 3*0x25800);

	// (img_offset-0x25800*n)/0x640, the images being below 2^21 bytes
	*frame = dRMath_div15((img_offset-0x25800*n) >> 6, 25);

	return race_car_shape[n];
}

static inline const __BYTE__ * race_car_box(__DWORD__ img_offset){

	int 	frame;
	const race_car_shapes_t * 	s = race_car_shape_of(img_offset, &frame);

	return s->box[frame];
}

static inline const __BYTE__ * race_car_rows(__DWORD__ img_offset){

	int 	frame;
	const race_car_shapes_t * 	s = race_car_shape_of(img_offset, &frame);

	return s->rows[frame][0];
}

// what race_main() does to the race's state before its first frame, with
// the track and cars from the given loaders: race_loaders_files reads the
// game's files
typedef struct {
	void 	(*inf)(void);		// the track's size, start places, power-ups and spectators
	void 	(*alloc)(void);		// the track's and the cars' buffers
	void 	(*load)(void);		// what goes in them, and the tables
	void 	(*scene)(void);		// the buildings and a reversed track, or 0
} race_loaders_t;
void race___49bd4h(void);
void race___42824h(void);
void race_inf_apply(const __BYTE__ * inf);
extern const race_loaders_t race_loaders_files;
void race_setup(int MyIndex, int NumCars, const race_loaders_t * load);

// race_loaders_baked: the track and the cars already decoded, as tools/gbabake
// writes them, from wherever race_baked points. The track buffers are used
// where they are, so on the GBA they stay in ROM. Without a scene step,
// the buildings, which are only drawn, and reversed tracks aren't baked yet
#define RACE_CAR_MODELS		7		// KUPLA ... SPECIAL, then the -S variants of the first 6
#define RACE_CAR_VARIANTS	(RACE_CAR_MODELS+6)
typedef struct race_baked_s {
	const __BYTE__ *	inf;		// TRn-INF.BIN
	const __BYTE__ *	mas;		// TRn-MAS.BPK, -VAI.BPK and -LR1.BPK decoded
	const __BYTE__ *	vai;
	const __BYTE__ *	lr1;
	const __BYTE__ *	ski_tab;	// TRn-SKI.TAB, -BLO.TAB, -LIT.TAB and VARJO.TAB, 0x100 bytes each
	const __BYTE__ *	blo_tab;
	const __BYTE__ *	lit_tab;
	const __BYTE__ *	varjo_tab;
	const __BYTE__ *	drv_dat;	// TRn-DRV.DAT, 0x800 bytes
	const __BYTE__ *	ohi_dat;	// TRn-OHI.DAT, 0x400 bytes
	const __BYTE__ *	mines1a;	// MINES1A.BPK and BLOWI.BPK decoded
	const __BYTE__ *	blowi;
	const __BYTE__ *	obstacle;	// OBSTACLE.BPK decoded, copied to obstacles
	const __BYTE__ *	cars;		// RACE_CAR_VARIANTS car_spins_t, not recolored
	const race_car_shapes_t * shapes;	// and their shapes
	const __BYTE__ *	tiles;		// track.tiles: the track image as 8x8 tiles, row by row
	const __BYTE__ *	sprites;	// sprites.bin: race_sprite_files[] decoded
	__POINTER__ 		ima;		// TRX_IMA, which the logic draws skid marks into
	__BYTE__ *		obstacles;	// 0x800 bytes, the logic changes them
} race_baked_t;
extern const race_baked_t * race_baked;
extern const race_loaders_t race_loaders_baked;
int race_car_variant(int n);

// race_quick.c: the menus' part of a race, for race_setup(MyIndex 0, 4 cars)
void race_quick(int track, const int models[4], const __BYTE__ colors[4][3], __DWORD__ seed);

// bss.c: 1024*cos and 1024*sin tables, 3601 entries
extern int * ___243310h;
extern int * ___243314h;

#endif // __DRALLY_RACE_H
