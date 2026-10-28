#include "drally.h"
#include "drmath.h"
#include "drally_structs_free.h"
#include "drally_race.h"

	extern __BYTE__ NUM_OF_CARS[];
	extern __BYTE__ ___1e6ed0h[];
	extern __BYTE__ ___1de580h[];
	extern __BYTE__ ___2438cch[];


static __DWORD__ helper00(int i, float f){ 
	
	return 1+!((((double)i)*((double)f)) > 0.0);
}

static double helper01(__DWORD__ val){

	__DWORD__	eax, edx, cf;
	__BYTE__ 	esp[4] __attribute__((aligned(4)));	// D() loads words here, see aligned.mk

	eax = val;
	edx = eax;
	edx = (int)edx>>0x1f;
	cf = !!(edx&0x40000000);
	edx <<= 0x2;
	eax -= edx+cf;
	eax = (int)eax>>0x2;

	F32(esp) = (float)(double)(int)eax;
	if((int)D(esp) > 0x40400000) D(esp) = 0x40400000;		// 3.0

	return (double)F32(esp);
}

// the last of the pixels in rows j0 to j1 and columns i0 to i1 that is
// solid in both cars' images: its row, and its column in *col, or 0
DR_HOT static int helper02(__BYTE__ * pn, __BYTE__ * pk, int j0, int j1, int i0, int i1, int * col){

	int 	i, j;

	for(j = j1; j >= j0; j--){
		for(i = i1; i >= i0; i--){

			if((pn[0x28*j+i] > 3)&&(pk[0x28*j+i] > 3)){

				*col = i;

				return j;
			}
		}
	}

	return 0;
}

static int max3(int a, int b, int c){ return (a > b) ? ((a > c) ? a : c) : ((b > c) ? b : c); }
static int min3(int a, int b, int c){ return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c); }

// RACE COLLISIONS
void race___4dcach(void){

	__DWORD__ 		eax, edx;
	__BYTE__ 		esp[0xd4] __attribute__((aligned(4)));	// D() loads words here, see aligned.mk
	int 			bl1, bl2;
	int 			n, k, i, j, l;
	struct_35e_t * 	s_35e;


	s_35e = (struct_35e_t *)___1e6ed0h;

	n = -1;
	while(++n < (int)D(NUM_OF_CARS)){

		s_35e[n].__fc = dRMath_divdf(s_35e[n].__fc, 1.15, 1.0/1.15);
		s_35e[n].__100 = dRMath_divdf(s_35e[n].__100, 1.15, 1.0/1.15);
		s_35e[n].__104 = dRMath_divdf(s_35e[n].__104, 1.2, 1.0/1.2);
	}
	
	n = -1;
	while(++n < (int)(D(NUM_OF_CARS)-1)){

		k = n;
		while(++k < (int)D(NUM_OF_CARS)){

			D(esp+0x68) = (int)((double)s_35e[k].XLocation-(double)s_35e[n].XLocation);
			D(esp+0x64) = (int)((double)s_35e[k].YLocation-(double)s_35e[n].YLocation);

			if((dRMath_abs_i(D(esp+0x68)) < 39)&&(dRMath_abs_i(D(esp+0x68)) < 39)){
			
				D(esp+0x94) = 0;

				// the last pixel, in rows from 1 and columns from 0, where
				// both cars' images are solid: searched from the end, so
				// the search stops at the first
				{
					int 	dx = D(esp+0x68), dy = D(esp+0x64);
					int 	w = 0x28-dRMath_abs_i(dx), h = 0x28-dRMath_abs_i(dy);
					__BYTE__ * 	pn = race_car_img(s_35e[n].ImgOffset)+0x28*dRMath_abspos_i(dy)+dRMath_abspos_i(dx);
					__BYTE__ * 	pk = race_car_img(s_35e[k].ImgOffset)+0x28*dRMath_absneg_i(dy)+dRMath_absneg_i(dx);

					// rows from 1 and columns from 0 of the overlap, inside
					// both images' boxes of solid pixels
					const __BYTE__ * 	bn = race_car_box(s_35e[n].ImgOffset);
					const __BYTE__ * 	bk = race_car_box(s_35e[k].ImgOffset);
					int 	pdx = dRMath_abspos_i(dx), pdy = dRMath_abspos_i(dy), ndx = dRMath_absneg_i(dx), ndy = dRMath_absneg_i(dy);
					int 	j0 = max3(1, bn[2]-pdy, bk[2]-ndy), j1 = min3(h-1, bn[3]-pdy, bk[3]-ndy);
					int 	i0 = max3(0, bn[0]-pdx, bk[0]-ndx), i1 = min3(w-1, bn[1]-pdx, bk[1]-ndx);

					if((j = helper02(pn, pk, j0, j1, i0, i1, &i)) > 0){

						D(esp+0x94) = 1;
						s_35e[n].__172 = i+dRMath_abspos_i(dx)-0x14;
						s_35e[n].__176 = j+dRMath_abspos_i(dy)-0x14;
						s_35e[k].__172 = i+dRMath_absneg_i(dx)-0x14;
						s_35e[k].__176 = j+dRMath_absneg_i(dy)-0x14;
					}
				}

				if(D(esp+0x94) == 1){

					D(esp+0x28) = (int)(((double)(int)(0-s_35e[n].__172)*((double)s_35e[k].YLocation-(double)s_35e[k].__152))+(((double)s_35e[k].XLocation-(double)s_35e[k].__14e)*(double)(int)s_35e[n].__176));

					s_35e[n].__100 = (float)(0.7*(double)((float)(((double)s_35e[k].YLocation-(double)s_35e[k].__152)-((double)s_35e[n].YLocation-(double)s_35e[n].__152)))+(double)s_35e[n].__100);

					s_35e[n].__fc = (float)((0.7*(((double)s_35e[k].XLocation-(double)s_35e[k].__14e)-((double)s_35e[n].XLocation-(double)s_35e[n].__14e)))+(double)s_35e[n].__fc);

					D(esp+0x2c) = (int)((((double)s_35e[n].YLocation-(double)s_35e[n].__152)*(double)(int)(0-s_35e[k].__172))+(((double)s_35e[n].XLocation-(double)s_35e[n].__14e)*(double)(int)s_35e[k].__176));

					s_35e[n].__18 = helper00(s_35e[n].__172, s_35e[n].__15a);
					s_35e[n].__1c = helper00(s_35e[n].__176, s_35e[n].__15e);
					
					s_35e[k].__fc = (float)((0.7*(((double)s_35e[n].XLocation-(double)s_35e[n].__14e)-((double)s_35e[k].XLocation-(double)s_35e[k].__14e)))+(double)s_35e[k].__fc);

					s_35e[k].__100 = (float)((0.7*(((double)s_35e[n].YLocation-(double)s_35e[n].__152)-((double)s_35e[k].YLocation-(double)s_35e[k].__152)))+(double)s_35e[k].__100);
					
					s_35e[k].__18 = helper00(s_35e[k].__172, s_35e[k].__15a);
					s_35e[k].__1c = helper00(s_35e[k].__176, s_35e[k].__15e);

					bl1 = !((s_35e[n].__18 != 1)||(s_35e[n].__18 != s_35e[k].__18));
					bl2 = !((s_35e[n].__1c != 1)||(s_35e[n].__1c != s_35e[k].__1c));

					if(bl1||bl2){

						if((double)s_35e[n].Direction <= (double)s_35e[k].Direction){

							s_35e[n].__104 = (float)((double)s_35e[n].__104+2.0);
							s_35e[k].__104 = (float)((double)s_35e[k].__104-2.0);
						}
						else {

							F32(esp+0x54) = (float)((double)s_35e[n].Direction-(double)s_35e[k].Direction);
							F32(esp+0x3c) = (0.0 <= (double)F32(esp+0x54))?F32(esp+0x54):(float)(-1.0*(double)F32(esp+0x54));

							if((int)D(esp+0x3c) >= 0x42c80000){	// 100.0

								s_35e[n].__104 = (float)((double)s_35e[n].__104+2.0);
								s_35e[k].__104 = (float)((double)s_35e[k].__104-2.0);
							}
							else {

								s_35e[n].__104 = (float)((double)s_35e[n].__104-2.0);
								s_35e[k].__104 = (float)((double)s_35e[k].__104+2.0);
							}
						}


						if((double)s_35e[n].XLocation < (double)s_35e[k].XLocation){

							s_35e[n].XLocation = (float)((double)s_35e[n].XLocation-0.6);
							s_35e[k].XLocation = (float)((double)s_35e[k].XLocation+0.6);
						}
						else {

							s_35e[n].XLocation = (float)((double)s_35e[n].XLocation+0.6);
							s_35e[k].XLocation = (float)((double)s_35e[k].XLocation-0.6);
						}


						if((double)s_35e[n].YLocation < (double)s_35e[k].YLocation){

							s_35e[n].YLocation = (float)((double)s_35e[n].YLocation-0.6);
							s_35e[k].YLocation = (float)((double)s_35e[k].YLocation+0.6);
						}
						else {

							s_35e[n].YLocation = (float)((double)s_35e[n].YLocation+0.6);
							s_35e[k].YLocation = (float)((double)s_35e[k].YLocation-0.6);
						}
					}

					F32(esp+0x30) = (float)dRMath_abs((double)s_35e[n].Direction-(double)s_35e[k].Direction);

					if(((int)D(esp+0x30) > 0x42340000 /*45.0*/)&&((int)D(esp+0x30) < 0x439d8000 /*315.0*/)){
						
						if(((int)D(esp+0x30) < 0x43070000 /*135.0*/)||((int)D(esp+0x30) > 0x43610000 /*225.0*/)){

							s_35e[n].__104 = (float)((double)s_35e[n].__104+helper01(D(esp+0x28)));
							s_35e[k].__104 = (float)((double)s_35e[k].__104+helper01(D(esp+0x2c)));
						}
					}

					D(esp+0x4c) = 0;

					if((D(___1de580h+0x94*n+0x24) != 0)&&(s_35e[k].Finished == 0)){

						eax = (int)(double)s_35e[k].XLocation-(int)((double)s_35e[n].XLocation+(double)s_35e[n].__cc);
						edx = (int)(double)s_35e[k].YLocation-(int)((double)s_35e[n].YLocation+(double)s_35e[n].__d0);	

						if((dRMath_abs_i(eax) < 0x14)&&(dRMath_abs_i(edx) < 0x14)){

							if(B(race_car_img(s_35e[k].ImgOffset)+(int)eax+0x28*(edx+0x14)+0x14) > 3) D(esp+0x4c) = 1;
						}

						eax = (int)(double)s_35e[k].XLocation-(int)((double)s_35e[n].XLocation+(double)s_35e[n].__c4);
						edx = (int)(double)s_35e[k].YLocation-(int)((double)s_35e[n].YLocation+(double)s_35e[n].__c8);

						if((dRMath_abs_i(eax) < 0x14)&&(dRMath_abs_i(edx) < 0x14)){

							if(B(race_car_img(s_35e[k].ImgOffset)+(int)eax+(int)(0x28*(edx+0x14))+0x14) > 3) D(esp+0x4c) = 1;
						}

						if(D(esp+0x4c) == 1){

							D(___2438cch) = (int)((double)(int)(0xc00-3*D(___1de580h+0x94*k+0x1c))*dRMath_sum2squares((double)s_35e[k].__fc, (double)s_35e[k].__100));
							if((int)D(___2438cch) > 0x2710) D(___2438cch) = 0x2710;
							D(___1de580h+0x94*k+0x18) -= D(___2438cch);
							if((int)D(___1de580h+0x94*k+0x18) < 0) D(___1de580h+0x94*k+0x18) = 0;
						}
					}
				}
			}
		}
	}
}
