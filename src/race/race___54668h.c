#include "drally.h"
#include "drally_structs_free.h"
#include "drmath.h"

	extern int NUM_OF_CARS;
	extern __BYTE__ ___243c60h[];
	extern __BYTE__ ___1e6ed0h[];
	extern __BYTE__ ___1de580h[];
	extern int MY_CAR_IDX;
	extern int TRX_WIDTH;
	extern __POINTER__ TRX_MAS;
	extern __POINTER__ TRX_IMA;
	extern __BYTE__ TRX_BLO_TAB[0x100];
	extern __BYTE__ ___2438d4h[];
	extern __BYTE__ TRX_SKI_TAB[0x100];

int rand_watcom106(void);

// a float's fraction is exact in float, and where it is 0.5 or more
// (int)(1.0+dval) is (int)dval+1: the same as in double
static int helper00(float fval){ return ((fval-(float)(int)fval) < 0.5f)?(int)fval:(int)fval+1; }

static void helper11(float fval1, float fval2, __BYTE__ * bp){

	int 	i, j;

	i = helper00(fval1);
	j = helper00(fval2);

	if((B(TRX_MAS+TRX_WIDTH*j+i)&0xf) == 0xf){

		__BYTE__* dest = TRX_IMA + TRX_WIDTH * j + i;
		dest[0] = bp[dest[0]];
		dest[1] = bp[dest[1]];
		dest += TRX_WIDTH;
		dest[0] = bp[dest[0]];
		dest[1] = bp[dest[1]];
	}
}

static float helper22(float fval){ return (0.0 <= (double)fval)?fval:(float)(-1.0*(double)fval); }

static int helper33(int idx, double dval){ 
	
	struct_94_t *	s_94 = (struct_94_t *)___1de580h;
	struct_35e_t * 	s_35e = (struct_35e_t *)___1e6ed0h;

	return (0.0 < (double)s_35e[idx].__b0)&&((double)s_35e[idx].__b0 < (dval*(double)s_94[idx].__4))&&((s_35e[idx].Ctrls[getCounter(4)]&1) != 0);
}

static int helper44(int idx){

	struct_35e_t * 	s_35e = (struct_35e_t *)___1e6ed0h;

	return (0.0 < (double)s_35e[idx].__b0)&&((s_35e[idx].Ctrls[getCounter(4)]&2) != 0);
}

// POSITION, LAP COUNTER
void race___54668h(void){

	int 			br0, k, n;
	__BYTE__ 		esp[0x124];
	float 			x1f, x2f, x3f, x4f, y1f, y2f, y3f, y4f;
	struct_35e_t * 	s_35e;
	struct_94_t *	s_94;

	s_35e = (struct_35e_t *)___1e6ed0h;
	s_94 = (struct_94_t *)___1de580h;

	n = -1;
	while(++n < NUM_OF_CARS){

		// the wheels, 12 pixels out at 22 degrees either side of the
		// direction and of the direction behind
		{
			int 	a_d = dRMath_turns_degf(s_35e[n].Direction), a22 = (int)((22u << 24)/360);
			float 	xm = dRMath_fixf(12*(dRMath_sin_i(a_d-a22) >> 4), 24), ym = dRMath_fixf(10*(dRMath_cos_i(a_d-a22) >> 4), 24);
			float 	xp = dRMath_fixf(12*(dRMath_sin_i(a_d+a22) >> 4), 24), yp = dRMath_fixf(10*(dRMath_cos_i(a_d+a22) >> 4), 24);

			s_35e[n].X1__10e = s_35e[n].XLocation-xm;
			s_35e[n].Y1__112 = s_35e[n].YLocation-ym;
			s_35e[n].X2__11e = s_35e[n].XLocation-xp;
			s_35e[n].Y2__122 = s_35e[n].YLocation-yp;
			s_35e[n].X3__12e = s_35e[n].XLocation+xm;
			s_35e[n].Y3__132 = s_35e[n].YLocation+ym;
			s_35e[n].X4__13e = s_35e[n].XLocation+xp;
			s_35e[n].Y4__142 = s_35e[n].YLocation+yp;
		}

		if(((int)s_94[n].__18 > 0)&&helper33(n, 0.55)) s_35e[n].__104 += (float)((double)(rand_watcom106()-rand_watcom106())/65536.0);

		if(((int)s_35e[n].__352 <= 0)||(s_94[MY_CAR_IDX].__2C == 0)){

			if(((double)(int)(s_94[n].__0+0xd) < (double)helper22(s_35e[n].__bc))||helper44(n)||helper33(n, 0.55)){

				if(((s_35e[n].Ctrls[getCounter(4)]&2) == 0)||((s_35e[n].Ctrls[getCounter(4)]&0x40) == 0)){

					if(MY_CAR_IDX == n){

						if((double)(int)(s_94[n].__0+0xd) < (double)helper22(s_35e[n].__bc)){

							D(___2438d4h) = (int)(2048.0*(double)helper22(s_35e[n].__bc));
						}

						if(((int)D(___2438d4h) > 0x10000)||helper44(n)||helper33(n, 0.85)) D(___2438d4h) = 0x10000;
					}

					s_35e[n].__34a++;

					if(s_35e[n].__34a == 6){
							
						br0 = 1;
						k = -1;
						while((++k < 0xf)&&br0){

							if(s_35e[n].__1e2[k] == 0){

								s_35e[n].__1e2[k] = 1;
								s_35e[n].__25a[k] = helper00(s_35e[n].X3__12e);
								s_35e[n].__2d2[k] = helper00(s_35e[n].Y3__132);
								br0 = 0;
							}

							if(s_35e[n].__21e[k] == 0){
									
								s_35e[n].__21e[k] = 1;
								s_35e[n].__296[k] = helper00(s_35e[n].X4__13e);
								s_35e[n].__30e[k] = helper00(s_35e[n].Y4__142);
								br0 = 0;
							}
						}

						s_35e[n].__34a = 0;
					}

					x1f = (s_35e[n].X1__10e-s_35e[n].X1__116)/(float)(int)s_35e[n].__b0;
					y1f = (s_35e[n].Y1__112-s_35e[n].Y1__11a)/(float)(int)s_35e[n].__b0;
					x2f = (s_35e[n].X2__11e-s_35e[n].X2__126)/(float)(int)s_35e[n].__b0;
					y2f = (s_35e[n].Y2__122-s_35e[n].Y2__12a)/(float)(int)s_35e[n].__b0;
					x3f = (s_35e[n].X3__12e-s_35e[n].X3__136)/(float)(int)s_35e[n].__b0;
					y3f = (s_35e[n].Y3__132-s_35e[n].Y3__13a)/(float)(int)s_35e[n].__b0;
					x4f = (s_35e[n].X4__13e-s_35e[n].X4__146)/(float)(int)s_35e[n].__b0;
					y4f = (s_35e[n].Y4__142-s_35e[n].Y4__14a)/(float)(int)s_35e[n].__b0;

					// the same for the whole mark
					br0 = !helper33(n, 0.85);

					k = -1;
					while(++k < (int)s_35e[n].__b0){

						if(br0){

							helper11(s_35e[n].X1__116+(float)k*x1f, s_35e[n].Y1__11a+(float)k*y1f, TRX_SKI_TAB);
							helper11(s_35e[n].X2__126+(float)k*x2f, s_35e[n].Y2__12a+(float)k*y2f, TRX_SKI_TAB);
						}

						helper11(s_35e[n].X3__136+(float)k*x3f, s_35e[n].Y3__13a+(float)k*y3f, TRX_SKI_TAB);
						helper11(s_35e[n].X4__146+(float)k*x4f, s_35e[n].Y4__14a+(float)k*y4f, TRX_SKI_TAB);
					}
				}
			}
		}
		else {

			s_35e[n].__352--;

			x1f = (s_35e[n].X1__10e-s_35e[n].X1__116)/(float)(int)s_35e[n].__b0;
			y1f = (s_35e[n].Y1__112-s_35e[n].Y1__11a)/(float)(int)s_35e[n].__b0;
			x2f = (s_35e[n].X2__11e-s_35e[n].X2__126)/(float)(int)s_35e[n].__b0;
			y2f = (s_35e[n].Y2__122-s_35e[n].Y2__12a)/(float)(int)s_35e[n].__b0;
			x3f = (s_35e[n].X3__12e-s_35e[n].X3__136)/(float)(int)s_35e[n].__b0;
			y3f = (s_35e[n].Y3__132-s_35e[n].Y3__13a)/(float)(int)s_35e[n].__b0;
			x4f = (s_35e[n].X4__13e-s_35e[n].X4__146)/(float)(int)s_35e[n].__b0;
			y4f = (s_35e[n].Y4__142-s_35e[n].Y4__14a)/(float)(int)s_35e[n].__b0;

			k = -1;
			while(++k < (int)s_35e[n].__b0){

				helper11(s_35e[n].X1__116+(float)k*x1f, s_35e[n].Y1__11a+(float)k*y1f, TRX_BLO_TAB);
				helper11(s_35e[n].X2__126+(float)k*x2f, s_35e[n].Y2__12a+(float)k*y2f, TRX_BLO_TAB);
				helper11(s_35e[n].X3__136+(float)k*x3f, s_35e[n].Y3__13a+(float)k*y3f, TRX_BLO_TAB);
				helper11(s_35e[n].X4__146+(float)k*x4f, s_35e[n].Y4__14a+(float)k*y4f, TRX_BLO_TAB);
			}
		}

		s_35e[n].X1__116 = s_35e[n].X1__10e;
		s_35e[n].Y1__11a = s_35e[n].Y1__112;
		s_35e[n].X2__126 = s_35e[n].X2__11e;
		s_35e[n].Y2__12a = s_35e[n].Y2__122;
		s_35e[n].X3__136 = s_35e[n].X3__12e;
		s_35e[n].Y3__13a = s_35e[n].Y3__132;
		s_35e[n].X4__146 = s_35e[n].X4__13e;
		s_35e[n].Y4__14a = s_35e[n].Y4__142;
	}
}
