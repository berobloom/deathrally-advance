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

// race_alloc()'s buffers the logic uses; the rest are only drawn from
static void baked_alloc(void){

	TRX_IMA = race_baked->ima;
	TRX_MAS = (__POINTER__)race_baked->mas;
	TRX_VAI = (__POINTER__)race_baked->vai;
	TRX_LR1 = (__POINTER__)race_baked->lr1;
	RACE_OBSTACLE_BPK = race_baked->obstacles;
	RACE_MINES1A_BPK = (__POINTER__)race_baked->mines1a;
	RACE_BLOWI_BPK = (__POINTER__)race_baked->blowi;
}

// race___42824h() without what is only drawn. The cars aren't recolored:
// the logic only asks whether a pixel is more than 3, which the body
// colors are before and after
static void baked_load(void){

	int 	n;

	D(___243d08h) = 0x100;

	// race_walls_build() looks at all 4, racing or not
	for(n = 0; n < 4; n++){

		race_car_slot[n] = (__BYTE__ *)race_baked->cars+((n < NUM_OF_CARS) ? race_car_variant(n) : 0)*sizeof(car_spins_t);
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
