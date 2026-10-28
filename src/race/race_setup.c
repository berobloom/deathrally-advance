#include "drally.h"
#include "drally_race.h"
#include "drally_structs_free.h"

#if defined(DR_MULTIPLAYER)
	extern __DWORD__ ___19bd60h;
	extern __BYTE__ ___243d08h[];
	extern __BYTE__ ___243d0ch[];
	extern __BYTE__ ___24387ch[];
	extern __BYTE__ ___243880h[];
	extern __BYTE__ ___243318h[];
	extern __BYTE__ ___243cd8h[];
	extern __BYTE__ ___243878h[];
	extern __BYTE__ ___1de580h[];
#endif // DR_MULTIPLAYER
	extern __DWORD__ MY_CAR_IDX;
	extern int NUM_OF_CARS;
	extern __BYTE__ ___243ca0h[];
	extern __BYTE__ ___243cdch[];
	extern __BYTE__ ___243cb8h[];
	extern __BYTE__ ___243cd4h[];
	extern __BYTE__ SUPERGLOBAL___243cd0h[];

void race___3f970h(void);
void race___49a34h(void);
void race___4a25ch(void);
void race___405bch(void);
void race___4083ch(void);
void race___496b0h(void);
#if defined(DR_MULTIPLAYER)
void srand_watcom106(__DWORD__);
#endif // DR_MULTIPLAYER


// everything race_main() does to the race's state before its first frame,
// in the same order, without the screen and the sound. The track and the
// cars come from the loaders: the desktop reads the game's files, the GBA
// points into data baked into its ROM. The racers are in ___1de7d0h, the
// track's name in ___19bd64h, as the menus leave them
void race_setup(int MyIndex, int NumCars, const race_loaders_t * load){

	MY_CAR_IDX = MyIndex;
	NUM_OF_CARS = NumCars;
	race___3f970h();
	race___49a34h();

#if defined(DR_MULTIPLAYER)
	if(___19bd60h != 0){

		int 			n;
		struct_94_t * 	s_94 = (struct_94_t *)___1de580h;

		D(___243d08h) = 0;
		D(___243d0ch) = 0;

		n = -1;
		while(++n < NUM_OF_CARS){

			D(___243d0ch) += s_94[n].__38;
			D(___243d0ch) += s_94[n].__3c;
			D(___243d0ch) += s_94[n].__40;
			D(___243d08h) += 1;
		}

		srand_watcom106(D(___243d0ch));
		D(___24387ch) = 0;
		D(___243880h) = 1;
		D(___243318h) = 0;
		D(___243cd8h) = 0;
		D(___243878h) = 0;
	}
#endif // DR_MULTIPLAYER

	load->inf();
	load->alloc();
	load->load();
	race___4a25ch();
	race___405bch();
	race___4083ch();
	if(load->scene) load->scene();
	race___496b0h();
	race___405bch();
	race___49a34h();
	race_walls_build();
	resetCounter(1);
	D(___243ca0h) = 0;
	D(___243cdch) = 0;
	D(___243cb8h) = 0;
	D(___243cd4h) = 0;
	resetCounter(5);
	resetCounter(2);
	D(SUPERGLOBAL___243cd0h) = 0;
}
