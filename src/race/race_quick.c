#include "drally.h"
#include "drally_race.h"

	extern char ___19bd64h[16];
	extern __BYTE__ ___1de7d0h[];
	extern int NUM_OF_LAPS;
	extern int LAP_RECORD_MIN;
	extern int LAP_RECORD_SEC;
	extern int LAP_RECORD_100;
	extern const char name_list[0x14][0xb];

void srand_watcom106(unsigned int seed);

// what race_setup() takes from the menus, which are all of its inputs
// besides the track and the cars: the racers in ___1de7d0h (0x54 bytes
// each, as ___33010h_cdecl() fills them), the track's name, the laps, the
// lap record and the random seed. A quick race: you in place 0 against
// three of the game's drivers, the four cars in the given models, no
// upgrades, no spikes, 8 mines each, no lap record. The others drive at
// difficulty 1, the setting of the games recorded so far; your car gets 3,
// as the menus give it. colors[n] are car n's body colors, 6-bit RGB
void race_quick(int track, const int models[4], const __BYTE__ colors[4][3], __DWORD__ seed){

	int 		n;
	__BYTE__ * 	r;

	memset(___1de7d0h, 0, 4*0x54);
	for(n = 0; n < 4; n++){

		r = ___1de7d0h+0x54*n;
		strcpy((char *)r, n ? name_list[n*5] : "YOU");
		D(r+0x28) = 8;				// mines
		D(r+0x2c) = 1;				// weapons on
		D(r+0x30) = models[n];
		D(r+0x34) = colors[n][0];
		D(r+0x38) = colors[n][1];
		D(r+0x3c) = colors[n][2];
		D(r+0x44) = n ? 1 : 3;
	}

	sprintf(___19bd64h, "TR%d", track);
	NUM_OF_LAPS = 4;
	LAP_RECORD_MIN = 0;
	LAP_RECORD_SEC = 0;
	LAP_RECORD_100 = 0;
	srand_watcom106(seed);
}
