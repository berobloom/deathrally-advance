#ifndef __DRALLY_RACE_H
#define __DRALLY_RACE_H

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
void race_walls_build(void);
void race_walls_free(void);
int race_walls_near(int x, int y, int w, int h);
unsigned race_walls_cells(int x, int y, int w, int h);
const __BYTE__ * race_car_box(__DWORD__ img_offset);

// car n's 96 rotation frames, 40x40 each, start at race_car_slot[n]. The
// desktop has the 4 one after the other at ___243d74h; the GBA points
// each into its ROM. A car's ImgOffset is 0x25800*n+0x640*frame
extern __BYTE__ * race_car_slot[4];
void race_car_slots_contiguous(void);
static inline __BYTE__ * race_car_img(__DWORD__ img_offset){

	int 	n = (img_offset >= 0x25800)+(img_offset >= 2*0x25800)+(img_offset >= 3*0x25800);

	return race_car_slot[n]+(img_offset-0x25800*n);
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
	__POINTER__ 		ima;		// TRX_IMA, which the logic draws skid marks into
	__BYTE__ *		obstacles;	// 0x800 bytes, the logic changes them
} race_baked_t;
extern const race_baked_t * race_baked;
extern const race_loaders_t race_loaders_baked;
int race_car_variant(int n);

// bss.c: 1024*cos and 1024*sin tables, 3601 entries
extern int * ___243310h;
extern int * ___243314h;

#endif // __DRALLY_RACE_H
