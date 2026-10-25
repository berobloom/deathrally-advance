#include "drally.h"
#include "drally_race.h"
#include "drally_structs_free.h"

#if defined(DR_MULTIPLAYER)
	extern __DWORD__ ___19bd60h;
#endif // DR_MULTIPLAYER
	extern __DWORD__ MY_CAR_IDX;
	extern int NUM_OF_CARS;
	extern int TRX_WIDTH;
	extern __POINTER__ TRX_IMA;
	extern __POINTER__ BACKBUFFER;
	extern __BYTE__ ___1e6ed0h[];
	extern __BYTE__ ___243334h[];
	extern __BYTE__ ___243c60h[];
	extern __BYTE__ ___196dd4h[];
	extern int TRX_VIEWPORT_TL_X;
	extern int TRX_VIEWPORT_TL_Y;
	extern int CURRENT_VIEWPORT_W;
	extern int CURRENT_VIEWPORT_X;
	extern int CURRENT_VIEWPORT_CENTER_X;

void race___4207ch(void);
void race___40f48h(void);
void race___40db4h(void);
void race___42218h(void);
void race___56594h(void);
void race___51eb4h(void);
void race___50ba4h(void);
void race___53464h(void);
void race___53310h(void);
void race___51204h(void);
void race___50a48h(void);
void race___4f170h(void);
void race___51ce0h(void);
void race___4f300h(void);
void race___50ef4h(void);
void race___52d7ch(void);
void race___4f030h(void);
void race___4ee9ch(void);

// 0 runs the render functions for what they change besides the picture
// (the camera, animations, explosions, the lap times, the ticks a frame
// takes), without drawing anything: what a port that draws the race its
// own way, like the GBA's, needs. The race then goes as it does drawn
int race_draw = 1;

// a frame's rendering, after race_logic()
void race_render(void){

	int 	m, ecx;

	race___4ee9ch();

	if(race_draw){

		race___4f030h();

		ecx = CURRENT_VIEWPORT_W+4;
		ecx -= ecx&3;
		m = -1;
		while(++m < 0xc8){	// COPY TRACK BACKGROUND

			memcpy(BACKBUFFER+0x200*m+CURRENT_VIEWPORT_X+0x60, TRX_IMA+TRX_WIDTH*(TRX_VIEWPORT_TL_Y+m)+TRX_VIEWPORT_TL_X, ecx);
		}
	}

#if defined(DR_MULTIPLAYER)
	if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
		race___52d7ch();
#if defined(DR_MULTIPLAYER)
	}
#endif // DR_MULTIPLAYER
	D(___243c60h) = -1;
	while(++D(___243c60h) < NUM_OF_CARS) race___50ef4h();
	race___4f300h();	// RENDER CARS
	race___51ce0h();
	if(race_draw&&dRally_Race_getSettings(RACE_SHADOWS)) race___4f170h();
	D(___243c60h) = -1;
	while(++D(___243c60h) < NUM_OF_CARS) race___50a48h();
	D(___243c60h) = -1;
	while(++D(___243c60h) < NUM_OF_CARS) race___51204h();
	if(race_draw){

		race___53310h();	// SORT OBJECTS
		race___53464h();	// OBJECTS
	}
	D(___243c60h) = -1;
	while(++D(___243c60h) < NUM_OF_CARS) race___50ba4h();
	race___51eb4h();
	// the cheats come from the keyboard
#if defined(DR_MULTIPLAYER)
	if(race_draw&&(___19bd60h == 0)){
#else
	if(race_draw){
#endif // DR_MULTIPLAYER
		race___56594h();	// INRACE CHEATS
	}
	if((int)getCounter(1) < 0x122) race___42218h();
	(CURRENT_VIEWPORT_W != 0x140) ? race___40f48h() : race___40db4h();

	D(___243c60h) = -1;
	while(++D(___243c60h) < NUM_OF_CARS){

		if((((struct_35e_t *)___1e6ed0h)[D(___243c60h)].Position == 1)&&(D(___196dd4h) != 0)) race___4207ch();
	}
}

// after the frame is on screen: the drugs wear off and the view widens or
// narrows to what the settings want
void race_render_after(void){

	int 	n;
	struct_35e_t * 	s_35e = (struct_35e_t *)___1e6ed0h;

	D(___243c60h) = 0;
	n = -1;
	while(++n < 4){

		if((int)s_35e[n].Drug > 0) s_35e[n].Drug -= D(___243334h);
	}

	D(___243c60h) = 4;

	if(dRally_Race_getSettings(RACE_NOTWIDE)){

		if(CURRENT_VIEWPORT_W > 0x100) CURRENT_VIEWPORT_W -= 4*D(___243334h);
		if(CURRENT_VIEWPORT_W < 0x100) CURRENT_VIEWPORT_W = 0x100;
	}
	else {

		if(CURRENT_VIEWPORT_W < 0x140) CURRENT_VIEWPORT_W += 2*D(___243334h);
		if(CURRENT_VIEWPORT_W > 0x140) CURRENT_VIEWPORT_W = 0x140;
	}

	CURRENT_VIEWPORT_CENTER_X = CURRENT_VIEWPORT_W/2;
	CURRENT_VIEWPORT_X = 0x140-CURRENT_VIEWPORT_W;
}
