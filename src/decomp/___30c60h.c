#include "drally.h"
#include "drally_structs_fixed.h"

	extern __BYTE__ ___1a01e0h[];
	extern __BYTE__ ___1a1ef8h[];
#if defined(DR_MULTIPLAYER)
	extern __DWORD__ ___19bd60h;
#endif // DR_MULTIPLAYER


// the first A1 racers in reverse order. Their copy is kept here rather than
// allocated, which the GBA, without the game's allocator, can't
DR_COLD static racer_t vp[20];

static void helper00(int A1){

	int 			n;
	racer_t * 		s_6c;

	s_6c = (racer_t *)___1a01e0h;

	memcpy(vp, s_6c, 20*sizeof(racer_t));

	n = -1;
	while(++n < A1) memcpy(&s_6c[n], &vp[A1-n-1], sizeof(racer_t));

	D(___1a1ef8h) = (A1-1)-D(___1a1ef8h);
}


// SOMETHING WITH RANKINGS SORTING 
void ___30c60h_cdecl(int A1){

	int 			n;
	racer_t * 		s_6c;

	s_6c = (racer_t *)___1a01e0h;
	helper00(A1);

	if(A1 > 0){
		
		s_6c[0].rank = 1;

		n = 0;
		while(++n < A1){

			if(s_6c[n-1].points != s_6c[n].points){

				s_6c[n].rank = n+1;
			}
			else {
			
				s_6c[n].rank = s_6c[n-1].rank;
			}
		}
	}

#if defined(DR_MULTIPLAYER)
	if(___19bd60h != 0) helper00(A1);
#endif // DR_MULTIPLAYER
}
