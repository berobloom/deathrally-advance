// The career's state, as the game's menus change it around a race: what
// ___33010h_cdecl() (PREPARE TO RACE), ___3266ch() (the sign-up) and
// ___3892ch_cdecl() (the results) do besides drawing and waiting for keys,
// moved here as they were so that the desktop's menus and the GBA's own
// screens share them. Each works on the racers in ___1a01e0h (racer_t), the
// round's races in ___1a1f64h (their tracks, then how many signed up for
// each) and ___1a0ef8h (who, 4 to a race), D(___1a1028h) (the race you're
// in, 0-2, 3 against the Adversary, -1 none) and D(___1a103ch) (your place
// on its grid).

#include "drally.h"
#include "drally_structs_fixed.h"
#include "drally_career.h"
#if defined(DR_MULTIPLAYER)
#include "drally_net.h"
#endif

	extern __BYTE__ ___1a01e0h[];
	extern __BYTE__ ___1a1ef8h[];
	extern __BYTE__ ___1a1028h[];
	extern __BYTE__ ___1a103ch[];
	extern __BYTE__ ___1a1f64h[];
	extern __BYTE__ ___1a0ef8h[];
	extern __BYTE__ ___1a0f04h[];
	extern __BYTE__ ___1de7d0h[];
	extern char ___19bd64h[16];
	extern int NUM_OF_LAPS;
	extern int LAP_BEST_SEC;
	extern int LAP_BEST_MIN;
	extern int LAP_BEST_100;
	extern int LAP_PREVIOUS_100;
	extern int LAP_PREVIOUS_SEC;
	extern int LAP_PREVIOUS_MIN;
	extern int LAP_RECORD_MIN;
	extern int LAP_RECORD_SEC;
	extern int LAP_RECORD_100;
	extern __BYTE__ ___1a1090h[];
	extern __BYTE__ ___1a10a8h[];
	extern __BYTE__ ___1a10a0h[];
	extern __BYTE__ ___196ab0h[];
	extern __BYTE__ ___196ab4h[];
	extern __BYTE__ ___1a2147h[];
	extern __BYTE__ ___196aa4h[];
	extern __DWORD__ ___196a94h_difficulty;
	extern __BYTE__ ___185a14h_UseWeapons[];
	extern __POINTER__ ___1a0fe0h;
	extern __POINTER__ ___1a0fb8h;
	extern record_t ___19f750h[6][18];
	extern __BYTE__ ___185a18h[];
	extern __BYTE__ ___185a44h[];
	extern __BYTE__ ___185a48h[];
	extern __BYTE__ ___196ae8h[];
	extern __BYTE__ ___196aa0h[];
	extern __BYTE__ ___196a9ch[];
	extern __BYTE__ ___196a98h[];
	extern __BYTE__ ___196aa8h[];
	extern __BYTE__ ___196aach[];
	extern __DWORD__ ___243d44h;
	extern __BYTE__ ___196af0h[];
	extern __BYTE__ ___196aech[];
#if defined(DR_MULTIPLAYER)
	extern __DWORD__ ___19bd60h;
	extern __DWORD__ MP_MONEY;
	extern __BYTE__ ___196ab8h[];
	extern __BYTE__ ___196ad4h[];
	extern __BYTE__ ___196a80h[];
	extern __BYTE__ ___196adch[];
	extern __BYTE__ ___1a2010h[];
	extern __BYTE__ ___18d492h[][0xf];
#endif // DR_MULTIPLAYER

char * strupr_watcom106(char * s);
int rand_watcom106(void);
void ___2ec68h_cdecl(void);
void ___3deb8h(void);
void ___31588h(void);
void ___3079ch_cdecl(__DWORD__);

// each of the round's tracks, ___1a1f64h's numbers: 0-8 TR1-TR9, 9-17 the
// same reversed, 18 TR0
const char ___106cbh[19][4] = {
	"TR1","TR2","TR3","TR4","TR5","TR6","TR7","TR8","TR9",
	"TR1","TR2","TR3","TR4","TR5","TR6","TR7","TR8","TR9",
	"TR0" };

static int others_best(int n_racers){

	int 		n, best = 0;
	racer_t * 	s_6c = (racer_t *)___1a01e0h;

	n = -1;
	while(++n < n_racers){

		if((best < s_6c[n].points)&&(n != D(___1a1ef8h))) best = s_6c[n].points;
	}

	return best;
}

int career_leading(void){

	return others_best(0x14) < ((racer_t *)___1a01e0h)[D(___1a1ef8h)].points;
}

// the race D(___1a1028h): its track in ___19bd64h, reversed or not, its
// laps, and who's on its grid, in grid[]
void career_race_track(__BYTE__ grid[4]){

	int 	race = D(___1a1028h), k = (race == 3) ? 18 : B(___1a1f64h+race);

	D(___1de7d0h+0x48) = !!(k > 8);
	strcpy(___19bd64h, ___106cbh[k]);
	if(race < 3) NUM_OF_LAPS = 4+race;
	else {

		NUM_OF_LAPS = 9;
		D(___1de7d0h+0x48) = 0;
	}
	grid[0] = B(race*4+___1a0ef8h);
	grid[1] = B(race*4+___1a0ef8h+1);
	grid[2] = B(race*4+___1a0ef8h+2);
	grid[3] = B(race*4+___1a0ef8h+3);
	if(race == 3){

		grid[0] = D(___1a1ef8h);
		grid[1] = D(___1a1ef8h);
	}
}

// the racers on grid[] into ___1de7d0h, as race_setup() takes them, and
// the lap times, the lap record and the bonus each kill brings
void career_race_fill(int NumCars, const __BYTE__ grid[4]){

	__DWORD__ 	eax, edx;
	__BYTE__ 	esp[0xd0];
	int 		n;
	__POINTER__ 	eaxp;
	racer_t * 	s_6c;

	s_6c = (racer_t *)___1a01e0h;
	memcpy(esp+0xcc, grid, 4);

	LAP_BEST_SEC = 0;
	LAP_BEST_MIN = 0;
	LAP_PREVIOUS_100 = 0;
	LAP_PREVIOUS_SEC = 0;
	LAP_PREVIOUS_MIN = 0;
	D(___1a1090h) = 0;
	D(___1a10a8h) = 0;
	D(___1a10a0h) = 0;
	D(___196ab0h) = 0;
	LAP_BEST_100 = 0;

	edx = 0;
	n = -1;
	while(++n < 0x14){

		if(((int)edx < (int)s_6c[n].points)&&(n != D(___1a1ef8h))) edx = s_6c[n].points;
	}

	D(___1de7d0h+0x4c) = 0;

#if defined(DR_MULTIPLAYER)
		if(___19bd60h == 0){
#endif
			if((int)D(___196aa4h) > 0) D(___1de7d0h+0x4c) = 1;
#if defined(DR_MULTIPLAYER)
		}
#endif // DR_MULTIPLAYER

	n = -1;
	while(++n < NumCars){

		strcpy(___1de7d0h+0x54*n, s_6c[B(esp+n+0xcc)].name);
		strupr_watcom106(___1de7d0h+0x54*n);
		D(___1de7d0h+0x54*n+0x10) = s_6c[B(esp+n+0xcc)].damage;
		D(___1de7d0h+0x54*n+0x14) = s_6c[B(esp+n+0xcc)].engine;
		D(___1de7d0h+0x54*n+0x18) = s_6c[B(esp+n+0xcc)].tires;
		D(___1de7d0h+0x54*n+0x30) = s_6c[B(esp+n+0xcc)].car;
		D(___1de7d0h+0x54*n+0x1c) = s_6c[B(esp+n+0xcc)].armor;

		edx = B(esp+n+0xcc);
		if(edx != D(___1a1ef8h)){

			D(___1de7d0h+0x54*n+0x44) = ___196a94h_difficulty;
		}
		else {
		
			D(___1de7d0h+0x54*n+0x44) = 3;
		}

		D(___1de7d0h+0x54*n+0x2c) = D(___185a14h_UseWeapons);
		D(___1de7d0h+0x54*n+0x20) = s_6c[B(esp+n+0xcc)].rocket_fuel;
		D(___1de7d0h+0x54*n+0x24) = s_6c[B(esp+n+0xcc)].spikes;
		D(___1de7d0h+0x54*n+0x28) = s_6c[B(esp+n+0xcc)].mines;		// LOAD BOUGHT MINES 

		edx = B(esp+n+0xcc);
		if(edx != D(___1a1ef8h)){
#if defined(DR_MULTIPLAYER)			
			if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
				D(___1de7d0h+0x54*n+0x28) = 0;
				D(___1de7d0h+0x54*n+0x24) = 0;
				D(___1de7d0h+0x54*n+0x20) = 0;
				if(((rand_watcom106()%5) == 0)&&(D(___185a14h_UseWeapons) != 0)) D(___1de7d0h+0x54*n+0x20) = 1;
				if(((rand_watcom106()%5) == 0)&&(D(___185a14h_UseWeapons) != 0)) D(___1de7d0h+0x54*n+0x24) = 1;
				if(((rand_watcom106()%5) == 0)&&(D(___185a14h_UseWeapons) != 0)) D(___1de7d0h+0x54*n+0x28) = 8;
#if defined(DR_MULTIPLAYER)	
			}
#endif // DR_MULTIPLAYER
		}

		eaxp = ___1a0fb8h;

#if defined(DR_MULTIPLAYER)	
		if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
			eax = B(esp+n+0xcc);
			if(eax != D(___1a1ef8h)) eaxp = ___1a0fe0h;
#if defined(DR_MULTIPLAYER)	
		}
#endif // DR_MULTIPLAYER

		D(___1de7d0h+0x54*n+0x34) = B(eaxp+3*s_6c[B(esp+n+0xcc)].color);
		D(___1de7d0h+0x54*n+0x38) = B(eaxp+3*s_6c[B(esp+n+0xcc)].color+1);
		D(___1de7d0h+0x54*n+0x3c) = B(eaxp+3*s_6c[B(esp+n+0xcc)].color+2);

#if defined(DR_MULTIPLAYER)
		if(___19bd60h != 0){

			if((int)(D(___196adch)-1) < n){

				D(___1de7d0h+0x54*n+0x34) = B(___1a0fe0h+0x1e);
				D(___1de7d0h+0x54*n+0x38) = B(___1a0fe0h+0x1f);
				D(___1de7d0h+0x54*n+0x3c) = B(___1a0fe0h+0x20);
			}
		}
#endif // DR_MULTIPLAYER
	}//w

#if defined(DR_MULTIPLAYER)
	if(___19bd60h != 0){

		n = -1;
		while(++n < 4){

			D(___1de7d0h+0x54*n+0x34) = B(___1a0fe0h+0x1e);
			D(___1de7d0h+0x54*n+0x38) = B(___1a0fe0h+0x1f);
			D(___1de7d0h+0x54*n+0x3c) = B(___1a0fe0h+0x20);
		}
	}
#endif // DR_MULTIPLAYER

	edx = 0;
	n = -1;
	while(++n < NumCars){

		if(((int)edx < (int)s_6c[n].points)&&(n != D(___1a1ef8h))) edx = s_6c[n].points;
	}

	if((int)edx < (int)s_6c[D(___1a1ef8h)].points){
#if defined(DR_MULTIPLAYR)
		if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
			strcpy(___1de7d0h, "ADVERSARY"); 
			D(___1de7d0h+0x44) = ___196a94h_difficulty;
			D(___1de7d0h+0x10) = 0;
			D(___1de7d0h+0x20) = 0;
			D(___1de7d0h+0x24) = 1;
			D(___1de7d0h+0x30) = 6;
			D(___1de7d0h+0x28) = 8*!!D(___185a14h_UseWeapons);
			D(___1de7d0h+0x2c) = D(___185a14h_UseWeapons);
			D(___1de7d0h+2*0x54+0x34) = B(___1a0fe0h+0x1e);
			D(___1de7d0h+2*0x54+0x38) = B(___1a0fe0h+0x1f);
			D(___1de7d0h+2*0x54+0x3c) = B(___1a0fe0h+0x20);
			D(___1de7d0h+3*0x54+0x34) = D(___1de7d0h+2*0x54+0x34);
			D(___1de7d0h+3*0x54+0x38) = D(___1de7d0h+2*0x54+0x38);
			D(___1de7d0h+3*0x54+0x3c) = D(___1de7d0h+2*0x54+0x3c);
#if defined(DR_MULTIPLAYR)
		}
#endif // DR_MULTIPLAYER
	}

	LAP_RECORD_MIN = ___19f750h[s_6c[D(___1a1ef8h)].car][B(___1a1f64h+D(___1a1028h))].min;
	LAP_RECORD_SEC = ___19f750h[s_6c[D(___1a1ef8h)].car][B(___1a1f64h+D(___1a1028h))].sec;
	LAP_RECORD_100 = ___19f750h[s_6c[D(___1a1ef8h)].car][B(___1a1f64h+D(___1a1028h))].sec100;
	D(___1de7d0h+0x50) = 0;

#if defined(DR_MULTIPLAYER)	
	if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
		if((int)edx >= (int)s_6c[D(___1a1ef8h)].points){

			if(D(___1a1028h) == 0) D(___1de7d0h+0x50) = 50;

			if(((int)s_6c[D(___1a1ef8h)].rank > 0)&&((int)s_6c[D(___1a1ef8h)].rank < 6)){

				if(D(___1a1028h) == 1) D(___1de7d0h+0x50) = 260;
				if(D(___1a1028h) == 2) D(___1de7d0h+0x50) = 500;
			}

			if(((int)s_6c[D(___1a1ef8h)].rank > 5)&&((int)s_6c[D(___1a1ef8h)].rank < 0xb)){

				if(D(___1a1028h) == 1) D(___1de7d0h+0x50) = 200;
				if(D(___1a1028h) == 2) D(___1de7d0h+0x50) = 300;
			}

			if(((int)s_6c[D(___1a1ef8h)].rank > 0xa)&&((int)s_6c[D(___1a1ef8h)].rank < 0x10)){

				if(D(___1a1028h) == 1) D(___1de7d0h+0x50) = 120;
				if(D(___1a1028h) == 2) D(___1de7d0h+0x50) = 150;
			}

			if(((int)s_6c[D(___1a1ef8h)].rank > 0xf)&&((int)s_6c[D(___1a1ef8h)].rank < 0x15)){

				if(D(___1a1028h) == 1) D(___1de7d0h+0x50) = 60;
				if(D(___1a1028h) == 2) D(___1de7d0h+0x50) = 80;
			}
		}
#if defined(DR_MULTIPLAYER)	
	}

	if(___19bd60h != 0) D(___1de7d0h+0x50) = MP_MONEY/0x32;

	if(___19bd60h == 0){
#endif // DR_MULTIPLAYER

		if((int)edx < (int)s_6c[D(___1a1ef8h)].points) D(___1de7d0h+0x50) = 0x190;
#if defined(DR_MULTIPLAYER)	
	}
#endif // DR_MULTIPLAYER
}

// after race_main(), which left each car's damage, place and kills in
// ___1de7d0h: the prize money and the bonuses, the wins, the damage taken
// and the races run, and a tip in the chat box
void career_race_after(int NumCars, const __BYTE__ grid[4]){

	__DWORD__ 	eax, ebx, edx;
	__BYTE__ 	esp[0xec];
	int 		n, bool_tmp;
	racer_t * 	s_6c;
#if defined(DR_MULTIPLAYER)
	char 		buffer[0x96];
#endif

	s_6c = (racer_t *)___1a01e0h;
	memcpy(esp+0xcc, grid, 4);

	D(___185a18h) = D(___196ae8h) = D(___1de7d0h+0x40+0x54*D(___1a103ch));

	edx = 0;
	n = -1;
	while(++n < 0x14){

		if((int)edx < (int)s_6c[n].points){
		
			if(n != D(___1a1ef8h)) edx = s_6c[n].points;
		}
	}

	bool_tmp = 0;
#if defined(DR_MULTIPLAYER)
	if(___19bd60h != 0) bool_tmp = 1;
#endif // DR_MULTIPLAYER

	if(((int)edx >= (int)s_6c[D(___1a1ef8h)].points)||bool_tmp){

		eax = 0;

		n = -1;
		while(++n < NumCars){

			if((n != D(___1a103ch))&&(D(___1de7d0h+0x54*n+0x10) == 100)) eax++;
		}

		if((eax == (NumCars-1))&&((int)D(___1de7d0h+0x54*D(___1a103ch)+0x10) < 100)) D(___196aa0h) = 1;

#if defined(DR_MULTIPLAYER)
		if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
			if((D(___196ae8h) != 4)&&((int)D(___1de7d0h+0x54*D(___1a103ch)+0x10) < 3)){
			
				if(D(___185a14h_UseWeapons) == 1) D(___196a9ch) = 1;
			}
#if defined(DR_MULTIPLAYER)
		}
		else {

			if(((int)D(___1de7d0h+0x54*D(___1a103ch)+0x10) < 3)||(D(___185a14h_UseWeapons) == 1)) D(___196a9ch) = 1;
		}
#endif // DR_MULTIPLAYER

		if(D(___196ae8h) != 1){

			D(___196a98h) = 0;
		}
		else {

			D(___196a98h)++;
		}

#if defined(DR_MULTIPLAYER)
		if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
			if((int)D(___196aa4h) > 0){

				if((D(___1de7d0h+0x4c) != 1)||(D(___1de7d0h+0x4c) != D(___196ae8h))){

					D(___196aa4h) = 0-D(___196aa4h);
				}
				else {

					if((int)D(___1de7d0h+0x54*D(___1a103ch)+0x10) >= 100) D(___196aa4h) = 0-D(___196aa4h);
				}
			}
#if defined(DR_MULTIPLAYER)
		}
			
		if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
			if((int)D(___196aa8h) > 0){

				edx = -1;
				while(++edx < 4){

					ebx = B(___1a0ef8h+4*D(___1a1028h)+edx);
					if(ebx == D(___196aach)) eax = edx;
				}

				if(D(___1de7d0h+0x54*eax+0x10) != 100) D(___196aa8h) = 0-D(___196aa8h);
			}
#if defined(DR_MULTIPLAYER)
		}
#endif // DR_MULTIPLAYER
	}

	if(s_6c[D(___1a1ef8h)].loanshark_type != -1) s_6c[D(___1a1ef8h)].loanshark_counter++;
	___243d44h = 0;

	n = -1;
	while(++n < 0x14){

		s_6c[n].spikes = 0;
		s_6c[n].rocket_fuel = 0;
		s_6c[n].sabotage = 0;
		s_6c[n].mines = 0;
	}

	___2ec68h_cdecl();
	D(___196af0h) = 0;
	D(___196aech) = 0;

	n = -1;
	while(++n < 4) s_6c[B(esp+n+0xcc)].bonus = 0;

	ebx = 0;
	n = -1;
	while(++n < 0x14){

		if(((int)ebx < (int)s_6c[n].points)&&(n != D(___1a1ef8h))) ebx = s_6c[n].points;
	}

#if defined(DR_MULTIPLAYER)
	if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
		if(((int)ebx >= (int)s_6c[D(___1a1ef8h)].points)&&(D(___196ab0h) == 0)){

			n = -1;
			while(++n < 4){

				if((D(___196ae8h) != 4)&&(D(___1a1028h) == 0)){

					s_6c[B(esp+n+0xcc)].bonus = 50*D(___1de7d0h+0x54*n+0xc);
					if(n == D(___1a103ch)) D(___196af0h) = s_6c[B(esp+n+0xcc)].bonus;
				}

				if(((int)s_6c[D(___1a1ef8h)].rank > 0)&&((int)s_6c[D(___1a1ef8h)].rank < 6)){

					if((D(___196ae8h) != 4)&&(D(___1a1028h) == 1)){
						
						s_6c[B(esp+n+0xcc)].bonus = 260*D(___1de7d0h+0x54*n+0xc);
						if(n == D(___1a103ch)) D(___196af0h) = s_6c[B(esp+n+0xcc)].bonus;
					}

					if((D(___196ae8h) != 4)&&(D(___1a1028h) == 2)){

						s_6c[B(esp+n+0xcc)].bonus = 500*D(___1de7d0h+0x54*n+0xc);
						if(n == D(___1a103ch)) D(___196af0h) = s_6c[B(esp+n+0xcc)].bonus;
					}
				}

				if(((int)s_6c[D(___1a1ef8h)].rank > 5)&&((int)s_6c[D(___1a1ef8h)].rank < 0xb)){

					if((D(___196ae8h) != 4)&&(D(___1a1028h) == 1)){

						s_6c[B(esp+n+0xcc)].bonus = 200*D(___1de7d0h+0x54*n+0xc);
						if(n == D(___1a103ch)) D(___196af0h) = s_6c[B(esp+n+0xcc)].bonus;
					}

					if((D(___196ae8h) != 4)&&(D(___1a1028h) == 2)){

						s_6c[B(esp+n+0xcc)].bonus = 300*D(___1de7d0h+0x54*n+0xc);
						if(n == D(___1a103ch)) D(___196af0h) = s_6c[B(esp+n+0xcc)].bonus;
					}
				}

				if(((int)s_6c[D(___1a1ef8h)].rank > 0xa)&&((int)s_6c[D(___1a1ef8h)].rank < 0x10)){

					if((D(___196ae8h) != 4)&&(D(___1a1028h) == 1)){

						s_6c[B(esp+n+0xcc)].bonus = 120*D(___1de7d0h+0x54*n+0xc);
						if(n == D(___1a103ch)) D(___196af0h) = s_6c[B(esp+n+0xcc)].bonus;
					}

					if((D(___196ae8h) != 4)&&(D(___1a1028h) == 2)){

						s_6c[B(esp+n+0xcc)].bonus = 150*D(___1de7d0h+0x54*n+0xc);
						if(n == D(___1a103ch)) D(___196af0h) = s_6c[B(esp+n+0xcc)].bonus;
					}
				}

				if(((int)s_6c[D(___1a1ef8h)].rank > 0xf)&&((int)s_6c[D(___1a1ef8h)].rank < 0x15)){

					if((D(___196ae8h) != 4)&&(D(___1a1028h) == 1)){

						s_6c[B(esp+n+0xcc)].bonus = 60*D(___1de7d0h+0x54*n+0xc);
						if(n == D(___1a103ch)) D(___196af0h) = s_6c[B(esp+n+0xcc)].bonus;
					}

					if((D(___196ae8h) != 4)&&(D(___1a1028h) == 2)){

						s_6c[B(esp+n+0xcc)].bonus = 80*D(___1de7d0h+0x54*n+0xc);
						if(n == D(___1a103ch)) D(___196af0h) = s_6c[B(esp+n+0xcc)].bonus;
					}
				}

				switch(D(___1a1028h)){
				case 0:
					if((D(___1de7d0h+0x54*n+0x40) == 1)&&(D(___1de7d0h+0x54*n+0x10) != 100)){

						s_6c[B(esp+n+0xcc)].bonus += 750;
						if(n == D(___1a103ch)) D(___196aech) = 750;
					}

					if((D(___1de7d0h+0x54*n+0x40) == 2)&&(D(___1de7d0h+0x54*n+0x10) != 100)){

						s_6c[B(esp+n+0xcc)].bonus += 375;
						if(n == D(___1a103ch)) D(___196aech) = 375;
					}

					if((D(___1de7d0h+0x54*n+0x40) == 3)&&(D(___1de7d0h+0x54*n+0x10) != 100)){
						
						s_6c[B(esp+n+0xcc)].bonus += 187;
						if(n == D(___1a103ch)) D(___196aech) = 187;
					}
					break;
				case 1:
					if((D(___1de7d0h+0x54*n+0x40) == 1)&&(D(___1de7d0h+0x54*n+0x10) != 100)){

						s_6c[B(esp+n+0xcc)].bonus += 3000;
						if(n == D(___1a103ch)) D(___196aech) = 3000;
					}

					if((D(___1de7d0h+0x54*n+0x40) == 2)&&(D(___1de7d0h+0x54*n+0x10) != 100)){

						s_6c[B(esp+n+0xcc)].bonus += 1500;
						if(n == D(___1a103ch)) D(___196aech) = 1500;
					}

					if((D(___1de7d0h+0x54*n+0x40) == 3)&&(D(___1de7d0h+0x54*n+0x10) != 100)){

						s_6c[B(esp+n+0xcc)].bonus += 375;
						if(n == D(___1a103ch)) D(___196aech) = 375;
					}
					break;
				case 2:
					if((D(___1de7d0h+0x54*n+0x40) == 1)&&(D(___1de7d0h+0x54*n+0x10) != 100)){

						s_6c[B(esp+n+0xcc)].bonus += 12000;
						if(n == D(___1a103ch)) D(___196aech) = 12000;
					}

					if((D(___1de7d0h+0x54*n+0x40) == 2)&&(D(___1de7d0h+0x54*n+0x10) != 100)){

						s_6c[B(esp+n+0xcc)].bonus += 6000;
						if(n == D(___1a103ch)) D(___196aech) = 6000;
					}

					if((D(___1de7d0h+0x54*n+0x40) == 3)&&(D(___1de7d0h+0x54*n+0x10) != 100)){

						s_6c[B(esp+n+0xcc)].bonus += 1500;
						if(n == D(___1a103ch)) D(___196aech) = 1500;
					}
					break;
				default:
					break;
				}

				s_6c[B(esp+n+0xcc)].money += s_6c[B(esp+n+0xcc)].bonus;
				s_6c[B(esp+n+0xcc)].income += s_6c[B(esp+n+0xcc)].bonus;
				if(D(___1de7d0h+0x54*n+0x40) == 1) s_6c[B(esp+n+0xcc)].wins++;
			}//w
		}//i
#if defined(DR_MULTIPLAYER)
	}

	if(___19bd60h == 0){
#endif // DR_MULTIPLAYER

		if((int)ebx < (int)s_6c[D(___1a1ef8h)].points){

			D(___196af0h) = 0x190*D(___1de7d0h+0x54*D(___1a103ch)+0xc);
			s_6c[B(esp+D(___1a103ch)+0xcc)].bonus = D(___196af0h);
		}
#if defined(DR_MULTIPLAYER)
	}

	if(___19bd60h != 0){

		D(___196ad4h)++;

		if((int)D(___196ad4h) < (int)D(___196ab8h)){

			if(D(___1de7d0h+0x44) != -1){

				strcpy(buffer, "-- ");
				strcat(buffer, ___18d492h[B(___1a2010h+D(___196ad4h))]);
				strcat(buffer, " race completed. Next race ");
				strcat(buffer, ___18d492h[B(___1a2010h+D(___196ad4h)+1)]);
				strcat(buffer, ".");
			}
			else {

				strcpy(buffer, "-- Multiplayer connection lost!");
			}
		}
		else {
		
			D(___196a80h) = 1;
			strcat(strcat(strcpy(buffer, "-- "), s_6c[D(___1a1ef8h)].name), " completed the netgame.");
		}

		dRChatbox_push(buffer, 0);
		dRChatbox_push("", 0);

		D(___196aech) = 0;
		D(___196af0h) = 0;

		n = -1;
		while(++n < (int)D(___196adch)){

			D(esp+0xd8) = MP_MONEY/50;
			s_6c[B(esp+n+0xcc)].bonus = D(___1de7d0h+0x54*n+0xc)*D(esp+0xd8);

			if(n == D(___1a103ch)) D(___196af0h) = s_6c[B(esp+n+0xcc)].bonus;

			if(D(___1de7d0h+0x54*n+0x40) == 1){

				D(esp+0xdc) = 6*MP_MONEY/20;
				s_6c[B(esp+n+0xcc)].bonus += D(esp+0xdc);
				if(n == D(___1a103ch)) D(___196aech) = D(esp+0xdc);
				s_6c[B(esp+n+0xcc)].points += 6;
			}

			if(D(___1de7d0h+0x54*n+0x40) == 2){

				D(esp+0xe0) = 5*MP_MONEY/20;
				s_6c[B(esp+n+0xcc)].bonus = D(esp+0xe0);
				if(n == D(___1a103ch)) D(___196aech) = D(esp+0xe0);
				s_6c[B(esp+n+0xcc)].points += 4;
			}

			if(D(___1de7d0h+0x54*n+0x40) == 3){

				D(esp+0xe4) = 4*MP_MONEY/20;
				s_6c[B(esp+n+0xcc)].bonus += D(esp+0xe4);
				if(n == D(___1a103ch)) D(___196aech) = D(esp+0xe4);
				s_6c[B(esp+n+0xcc)].points += 2;
			}

			if(D(___1de7d0h+0x54*n+0x40) == 4){

				D(esp+0xe8) = 3*MP_MONEY/20;
				s_6c[B(esp+n+0xcc)].bonus += D(esp+0xe8);
				if(n == D(___1a103ch)) D(___196aech) = D(esp+0xe8);
			}

			s_6c[B(esp+n+0xcc)].money += s_6c[B(esp+n+0xcc)].bonus;
			s_6c[B(esp+n+0xcc)].income += s_6c[B(esp+n+0xcc)].bonus;

			if(D(___1de7d0h+0x54*n+0x40) == 1) s_6c[B(esp+n+0xcc)].wins++;
		}//w
	}//i
#endif // DR_MULTIPLAYER

	edx = 0;
	n = -1;
	while(++n < 0x14){

		if(((int)edx < (int)s_6c[n].points)&&(n != D(___1a1ef8h))) edx = s_6c[n].points;
	}

#if defined(DR_MULTIPLAYER)
	if(___19bd60h == 0){
#endif // DR_MULTIPLAYER

		if((int)edx < (int)s_6c[D(___1a1ef8h)].points){
		
			s_6c[D(___1a1ef8h)].damage = D(___1de7d0h+0x54+0x10);
			s_6c[D(___1a1ef8h)].races++;
		}
		else {

			n = -1;
			while(++n < NumCars){

				s_6c[B(esp+n+0xcc)].damage = D(___1de7d0h+0x54*n+0x10);
				s_6c[B(esp+n+0xcc)].races++;
			}
		}
#if defined(DR_MULTIPLAYER)
	}
	else {

		n = -1;
		while(++n < NumCars){

			s_6c[B(esp+n+0xcc)].damage = D(___1de7d0h+0x54*n+0x10);
			s_6c[B(esp+n+0xcc)].races++;
		}
	}
	
	if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
		___3deb8h();
#if defined(DR_MULTIPLAYER)
	}
#endif // DR_MULTIPLAYER
}

// once you lead on points, after the race against the Adversary: the
// round goes on without you, its places at random. 1 if you beat him,
// which ends the game
int career_leader_round(void){

	__BYTE__ 	taken[4];
	int 		n, place;

	___31588h();
	while(1){

		___3079ch_cdecl(1);
		if((4 <= B(___1a1f64h+3))&&(4 <= B(___1a1f64h+4))&&(4 <= B(___1a1f64h+5))) break;
	}

	memset(taken, 0, 4);
	n = 0;
	while(n < 4){

		place = (rand_watcom106()%4)+1;
		D(___1de7d0h+0x54*n+0x40) = place;
		if(taken[place-1] == 0){

			taken[place-1] = 1;
			n++;
		}
	}

	return D(___196ae8h) == 1;
}

// the sign-up: why you can't sign up for race (0-2) yet, or 0 if you can:
// CAREER_FULL, or the first time, CAREER_NEEDS_CAR, for a better car
int career_signup_check(int race){

	racer_t * 	s_6c = (racer_t *)___1a01e0h;

	if((race == 1)&&(D(___185a44h) == 1)&&((int)s_6c[D(___1a1ef8h)].car < 2)){

		D(___185a44h) = 0;
		return CAREER_NEEDS_CAR;
	}
	if((race == 2)&&(D(___185a48h) == 1)&&((int)s_6c[D(___1a1ef8h)].car < 4)){

		D(___185a48h) = 0;
		return CAREER_NEEDS_CAR;
	}
	if(B(___1a1f64h+race+3) >= 4) return CAREER_FULL;

	return 0;
}

// you in race's next place
void career_signup(int race){

	int 	ebx = B(___1a1f64h+race+3);

	D(___1a1028h) = race;
	B(___1a0ef8h+4*race+ebx) = D(___1a1ef8h);
	B(___1a1f64h+race+3)++;
	D(___1a103ch) = ebx;
}

int career_signup_full(void){

	return (B(___1a1f64h+3) >= 4)&&(B(___1a1f64h+4) >= 4)&&(B(___1a1f64h+5) >= 4);
}

// the others signed up until every race has 4
void career_signup_rest(void){

	while(!career_signup_full()) ___3079ch_cdecl(1);
}

// once they all have: each race's grid ordered, the highest numbered
// racers (the lowest ranked) at the front, and your place on yours
void career_signup_order(void){

	__BYTE__ 	taken[0x14], in[4];
	int 		eax, edi, n, n2, n3;

	n2 = -1;
	while(++n2 < 3){

		memset(taken, 0, 0x14);
		memcpy(in, ___1a0ef8h+4*n2, 4);
		edi = 4*n2;
		n3 = -1;
		while(++n3 < 4){

			eax = 0;
			while(1){

				n = in[eax];
				eax++;
				if(taken[n] == 0) break;
			}
			eax = in[1];
			if((eax > n)&&(taken[eax] == 0)) n = eax;
			eax = in[2];
			if((eax > n)&&(taken[eax] == 0)) n = eax;
			eax = in[3];
			if((eax > n)&&(taken[eax] == 0)) n = eax;
			taken[n] = 1;
			B(___1a0ef8h+edi) = n;
			if(D(___1a1ef8h) == n) D(___1a103ch) = n3;
			edi++;
		}
	}
}


// a new game, as the main menu's START A NEW GAME leaves it: you (face 0-19,
// one of the drivers' faces, whom the game then leaves out) against the 19
// others, at difficulty 0-2
void career_new(const char * name, int face, int difficulty){

	extern __BYTE__ ___1a1efch[];
	extern __BYTE__ ___185a40h[];
	extern __BYTE__ ___185a3ch[];
	extern __BYTE__ ___185c0bh[];
	void ___2415ch(void);

	racer_t * 	s_6c = (racer_t *)___1a01e0h;
	char * 		me;
	int 		n;

	// ___3ab5ch_cdecl()'s, the name lower case but its first letter
	D(___1a1ef8h) = 0x13;
	me = s_6c[D(___1a1ef8h)].name;
	strncpy(me, name, sizeof(s_6c->name)-1);
	me[sizeof(s_6c->name)-1] = 0;
	for(n = 0; me[n]; n++) if((me[n] >= 'A')&&(me[n] <= 'Z')) me[n] += 0x20;
	if((me[0] > 0x60)&&(me[0] < 0x7b)) me[0] -= 0x20;
	D(___1a1efch) = 0;
	for(n = 0; me[n]; n++) D(___1a1efch) += B(___185c0bh+me[n]-0x1e);
	s_6c[D(___1a1ef8h)].face = face;
	s_6c[D(___1a1ef8h)].color = 0;
	___196a94h_difficulty = difficulty;

	// then menu___194a8h()'s
	D(___185a48h) = 1;
	D(___185a44h) = 1;
	D(___185a40h) = 1;
	D(___185a3ch) = 1;
	n = s_6c[D(___1a1ef8h)].color;
	___2415ch();
	s_6c[D(___1a1ef8h)].color = n;
	___243d44h = 1;
}

// the sign-up opening: the round's three races, nobody in them yet
void career_signup_open(void){

	D(___1a1028h) = -1;
	___31588h();
}

// your best lap on the race's track, the record for your car if it beats
// it, as ___36adch() keeps them
void career_lap_record(void){

	racer_t * 	s_6c = (racer_t *)___1a01e0h;
	record_t * 	r;
	int 		best;

	if((int)D(___196ae8h) <= 0) return;
	r = &___19f750h[s_6c[D(___1a1ef8h)].car][B(___1a1f64h+D(___1a1028h))];
	best = 0x64*LAP_BEST_SEC+0x1770*LAP_BEST_MIN+LAP_BEST_100;
	if((best < (int)(r->sec100+100*r->sec+6000*r->min))&&((LAP_BEST_MIN+LAP_BEST_SEC+LAP_BEST_100) != 0)){

		strupr_watcom106(strcpy(r->name, s_6c[D(___1a1ef8h)].name));
		r->min = LAP_BEST_MIN;
		r->sec = LAP_BEST_SEC;
		r->sec100 = LAP_BEST_100;
	}
	if(((r->min+r->sec+r->sec100) == 0)&&(best > 0)){

		strupr_watcom106(strcpy(r->name, s_6c[D(___1a1ef8h)].name));
		r->min = LAP_BEST_MIN;
		r->sec = LAP_BEST_SEC;
		r->sec100 = LAP_BEST_100;
	}
}

// you leading on points for the first time: B(___1a2147h), and the
// Adversary's challenge
void career_leader_check(void){

	int 	lead = career_leading();

	if(lead&&(D(___196ab4h) == 0)) B(___1a2147h) = 1;
	D(___196ab4h) = lead;
}

// ___3892ch_cdecl(A1)'s: the round's points, 0 after a race you were in,
// 1 after one you weren't, 2 without; the racers sorted on them, ranked,
// and your lap record. 1 if you lead for the first time
int career_results(int A1){

	void ___36068h_cdecl(__DWORD__);
	void ___36358h_cdecl(__DWORD__);
	void ___36718h_cdecl(__DWORD__);
	void ___30a84h_cdecl(__DWORD__, __DWORD__);
	void ___30c60h_cdecl(__DWORD__);
	extern __BYTE__ ___185a3ch[];
	extern __BYTE__ ___185a4ch[];

	B(___1a2147h) = 0;
	if(A1 != 2){

		___36068h_cdecl(0);
		___36358h_cdecl(0);
		___36718h_cdecl(0);
		___30a84h_cdecl(0, 0x13);
		___30c60h_cdecl(0x14);
	}
	career_lap_record();
	D(___196ae8h) = 0;
	if(A1 == 0) career_leader_check();
	if(B(___1a2147h) != 1) return 0;
	// the Adversary's film, then
	B(___1a2147h) = 0;
	if((A1 == 2)||((D(___185a14h_UseWeapons) != 0)&&(D(___185a3ch) == 0)&&(D(___185a4ch) != 0))){

		D(___196aa0h) = 0;
		D(___196a9ch) = 0;
		D(___196a98h) = 0;
	}

	return 1;
}

// the Adversary's challenge turned down (___31008h()): the others race the
// round without you, these places at random; then the results, as after a
// race you weren't in (A1 1), and career_adversary_declined()
void career_adversary_decline(void){

	__BYTE__ 	taken[4];
	int 		n, place;

	career_signup_rest();
	memset(taken, 0, 4);
	n = -1;
	while(++n < 4){

		while(1){

			place = 1+rand_watcom106()%4;
			D(___1de7d0h+0x54*n+0x40) = place;
			if(taken[place-1] == 0) break;
		}
		taken[place-1] = 1;
	}
}

void career_adversary_declined(void){

	extern __BYTE__ ___1a0a50h[];

	D(___1a0a50h+0xc) = !career_leading();
}

// the race against him: you second on a grid of 2, his car first
void career_adversary_accept(void){

	D(___1a1028h) = 3;
	D(___1a103ch) = 1;
	D(___196af0h) = 0;
	D(___196aech) = 0;
}
