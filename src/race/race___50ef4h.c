#include "drally.h"
#include "drally_race.h"
#include "drally_structs_free.h"

	extern __POINTER__ BACKBUFFER;
	extern __POINTER__ RACE_SMOKE_BPK;
	extern __BYTE__ ___243c60h[];
	extern __BYTE__ ___1e6ed0h[];
	extern int TRX_VIEWPORT_TL_X;
	extern int CURRENT_VIEWPORT_X;
	extern int TRX_VIEWPORT_TL_Y;
	extern __BYTE__ ___243334h[];

static void helper_copy(__POINTER__ dst, int dstX, int dstY, __POINTER__ src){

	int 		i, j;
	__BYTE__ 	px;

	__BYTE__ *s = src;
	__BYTE__ *d = dst + 0x200 * dstY + dstX;
	j = 8;
	do {
		i = 8;
		do {
			if ((px = *s++) != 0) *d = px;
			d++;
		} while (--i);
		d += (0x200 - 8);
	} while (--j);
}

// TIRE DUSTING
void race___50ef4h(void){

	int 			i0, j0, n, ticks;
	struct_35e_t * 	car;

	// read once: D() loads byte by byte, and again after every store
	car = &((struct_35e_t *)___1e6ed0h)[D(___243c60h)];
	ticks = D(___243334h);

	n = -1;
	while(++n < 15){

// LEFT
		if((int)car->__1e2[n] > 0){

			i0 = car->__25a[n]+CURRENT_VIEWPORT_X-TRX_VIEWPORT_TL_X;
			j0 = car->__2d2[n]-TRX_VIEWPORT_TL_Y;

			if((i0 >= 4 + CURRENT_VIEWPORT_X)&&((i0+4) < 0x140)&&(j0 >= 4)&&((j0+4) < 0xc8)){

				if(race_draw) helper_copy(BACKBUFFER, i0+0x5c, j0-4, RACE_SMOKE_BPK+0x40*((car->__1e2[n]-1)/4));
			}

			car->__1e2[n] += ticks;
			if((int)car->__1e2[n] > 0xc) car->__1e2[n] = 0;
		}

// RIGHT
		if((int)car->__21e[n] > 0){

			i0 = car->__296[n]+CURRENT_VIEWPORT_X-TRX_VIEWPORT_TL_X;
			j0 = car->__30e[n]-TRX_VIEWPORT_TL_Y;

			if((i0 >= 4 + CURRENT_VIEWPORT_X)&&((i0+4) < 0x140)&&(j0 >= 4)&&((j0+4) < 0xc8)){

				if(race_draw) helper_copy(BACKBUFFER, i0+0x5c, j0-4, RACE_SMOKE_BPK+0x40*((car->__21e[n]-1)/4));
			}

			car->__21e[n] += ticks;
			if((int)car->__21e[n] > 0xc) car->__21e[n] = 0;
		}
	}
}
