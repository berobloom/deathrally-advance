#include "drally.h"
#include "drally_race.h"
#include "drally_structs_free.h"

	extern __BYTE__ ___1de580h[];
	extern __BYTE__ ___1de7d0h[];
	extern __BYTE__ ___243d08h[];
	extern int NUM_OF_CARS;
	extern int TRX_WIDTH;
	extern int TRX_HEIGHT;
	extern __POINTER__ TRX_IMA;
	extern __POINTER__ TRX_MAS;
	extern __POINTER__ TRX_VAI;
	extern __POINTER__ TRX_LR1;
	extern __POINTER__ RACE_OBSTACLE_BPK;
	extern __POINTER__ RACE_MINES1A_BPK;
	extern __POINTER__ RACE_BLOWI_BPK;
	extern __POINTER__ RACE_SMOKE_BPK;
	extern __POINTER__ RACE_PEDESTR_BPK;
	extern __POINTER__ RACE_SPLATX_BPK[2];
	extern __POINTER__ RACE_ROCKETX_BPK[2];
	extern __POINTER__ RACE_BURN1A_BPK;
	extern __POINTER__ RACE_SHOTS_BPK;
	extern __POINTER__ RACE_FLAMEX_BPK[6];
	extern __POINTER__ GEN_LAM_BPK;
	extern __BYTE__ VARJO_TAB[0x100];
	extern __BYTE__ TRX_LIT_TAB[0x100];
	extern __BYTE__ TRX_BLO_TAB[0x100];
	extern __BYTE__ TRX_SKI_TAB[0x100];
	extern float TRX_DRV_DAT[0x200];
	extern int TRX_OHI_DAT[0x100];
	extern __DWORD__ NUM_OF_OBJECTS;
	extern __DWORD__ NUM_OF_TEXTURES;

const race_baked_t * race_baked;

// car n's images among race_baked->cars: its model, as race___42824h()
// picks the file, or past the 7 models the model's -S.BPK
int race_car_variant(int n){

	struct_54_t * 	s_54 = (struct_54_t *)___1de7d0h;
	int 		model = D(___1de580h+0x94*n);

	return ((s_54[n].__24 == 1)&&(model < 6)) ? RACE_CAR_MODELS+model : model;
}

static void baked_inf(void){

	race_inf_apply(race_baked->inf);
}

// how many of the track image's tiles a race can draw into on the GBA, 64
// bytes each: skid marks, mines and the rest stop showing past them
#if !defined(RACE_IMA_TILES)
#define RACE_IMA_TILES 		2048
#endif

// race_alloc()'s buffers the logic uses; the rest are only drawn from
static void baked_alloc(void){

#if defined(DR_IMA_TILES)
	race_ima_tiles(race_baked->tiles, (TRX_WIDTH+7) >> 3, (TRX_HEIGHT+7) >> 3, RACE_IMA_TILES);
#else
	TRX_IMA = race_baked->ima;
#endif
	TRX_MAS = (__POINTER__)race_baked->mas;
	TRX_VAI = (__POINTER__)race_baked->vai;
	TRX_LR1 = (__POINTER__)race_baked->lr1;
	RACE_OBSTACLE_BPK = race_baked->obstacles;
	RACE_MINES1A_BPK = (__POINTER__)race_baked->mines1a;
	RACE_BLOWI_BPK = (__POINTER__)race_baked->blowi;

	// the pictures drawn over the track, in race_sprite_files[]'s order
	if(race_baked->sprites){

		__POINTER__ * 	to[RACE_SPRITE_FILES] = {
			&RACE_SMOKE_BPK, &RACE_PEDESTR_BPK, &RACE_SPLATX_BPK[0], &RACE_SPLATX_BPK[1],
			&RACE_ROCKETX_BPK[0], &RACE_ROCKETX_BPK[1], &RACE_BURN1A_BPK, &RACE_SHOTS_BPK,
			&RACE_FLAMEX_BPK[0], &RACE_FLAMEX_BPK[1], &RACE_FLAMEX_BPK[2], &RACE_FLAMEX_BPK[3],
			&RACE_FLAMEX_BPK[4], &RACE_FLAMEX_BPK[5], &GEN_LAM_BPK
		};
		unsigned 	i, o;

		for(i = o = 0; i < RACE_SPRITE_FILES; o += race_sprite_files[i++].size) *to[i] = (__POINTER__)race_baked->sprites+o;
	}
}

// race___42824h() without what is only drawn. The cars aren't recolored:
// the logic only asks whether a pixel is more than 3, which the body
// colors are before and after
static void baked_load(void){

	int 	n;

	D(___243d08h) = 0x100;

	// race_walls_build() looks at all 4, racing or not
	for(n = 0; n < 4; n++){

		int 	v = (n < NUM_OF_CARS) ? race_car_variant(n) : 0;

		race_car_slot[n] = (__BYTE__ *)race_baked->cars+v*sizeof(car_spins_t);
		race_car_shape[n] = race_baked->shapes+v;
	}

	memcpy(race_baked->obstacles, race_baked->obstacle, 0x800);
	memcpy(TRX_SKI_TAB, race_baked->ski_tab, 0x100);
	memcpy(TRX_BLO_TAB, race_baked->blo_tab, 0x100);
	memcpy(TRX_LIT_TAB, race_baked->lit_tab, 0x100);
	memcpy(VARJO_TAB, race_baked->varjo_tab, 0x100);
	memcpy(TRX_DRV_DAT, race_baked->drv_dat, 0x800);
	memcpy(TRX_OHI_DAT, race_baked->ohi_dat, 0x400);

	// the track's buildings, which are only drawn
	NUM_OF_OBJECTS = 0;
	NUM_OF_TEXTURES = 0;
}

const race_loaders_t race_loaders_baked = { baked_inf, baked_alloc, baked_load, 0 };
