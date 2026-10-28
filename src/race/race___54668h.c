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

// rounds as ((d-(int)d) < 0.5) ? (int)d : (int)(1.0+d) in double did:
// halves up when positive, towards 0 when negative. From the float's bits,
// which soft-float would take ten times as long for; checked against the
// original for every 97th float below 2^31 in magnitude
static int helper00(float fval){

	union { float f; unsigned int u; } b;
	int 	e, m, r;

	b.f = fval;
	e = (int)((b.u >> 23)&0xff)-127;
	if(e < 0) return ((int)b.u >= 0)&&(e == -1);

	m = (b.u&0x7fffff)|0x800000;
	r = (e < 23) ? (m >> (23-e)) : (m << (e-23));
	if((int)b.u < 0) return -r;

	return (e < 23) ? r+((m >> (22-e))&1) : r;
}

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

// a mark of len points from (x, y) in steps of (dx, dy). The marks of a
// car's wheels darken the track through the same table, so drawing them
// one after the other gives what drawing them point by point together did
DR_HOT static void helper55(float x, float y, float dx, float dy, int len, __BYTE__ * bp){

	int 	k;

	for(k = 0; k < len; k++) helper11(x+(float)k*dx, y+(float)k*dy, bp);
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
	__BYTE__ 		esp[0x124] __attribute__((aligned(4)));	// D() loads words here, see aligned.mk
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

		if(((int)s_94[n].__18 > 0)&&helper33(n, 0.55)){

			// the two calls in order: C leaves the order of a-b's operands open
			int 	r = rand_watcom106();

			s_35e[n].__104 += (float)((double)(r-rand_watcom106())/65536.0);
		}

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

					k = (int)s_35e[n].__b0;
					if(br0){

						helper55(s_35e[n].X1__116, s_35e[n].Y1__11a, x1f, y1f, k, TRX_SKI_TAB);
						helper55(s_35e[n].X2__126, s_35e[n].Y2__12a, x2f, y2f, k, TRX_SKI_TAB);
					}
					helper55(s_35e[n].X3__136, s_35e[n].Y3__13a, x3f, y3f, k, TRX_SKI_TAB);
					helper55(s_35e[n].X4__146, s_35e[n].Y4__14a, x4f, y4f, k, TRX_SKI_TAB);
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

			k = (int)s_35e[n].__b0;
			helper55(s_35e[n].X1__116, s_35e[n].Y1__11a, x1f, y1f, k, TRX_BLO_TAB);
			helper55(s_35e[n].X2__126, s_35e[n].Y2__12a, x2f, y2f, k, TRX_BLO_TAB);
			helper55(s_35e[n].X3__136, s_35e[n].Y3__13a, x3f, y3f, k, TRX_BLO_TAB);
			helper55(s_35e[n].X4__146, s_35e[n].Y4__14a, x4f, y4f, k, TRX_BLO_TAB);
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
