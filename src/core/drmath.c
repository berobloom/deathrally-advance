#include "drally.h"
#include "drmath.h"

#include "drmath_sin.h"

// sin() and cos() from a table, which the GBA can afford 170 times a race
// tick where libm's took half the time, and which gives the same results
// on every CPU, so that the GBA's races stay the desktop's. They are off by
// at most 7e-7.
//
// Angles are in 1/2^24 turns; the table's 4096 steps are interpolated with
// the lower 12 bits.
int dRMath_sin_i(int a){

	int 	i = (a >> 12)&0xfff, f = a&0xfff;

	return dRMath_sin_tab[i]+(((dRMath_sin_tab[i+1]-dRMath_sin_tab[i])*f) >> 12);
}

int dRMath_cos_i(int a){

	return dRMath_sin_i(a+(1 << 22));
}

// the GBA does doubles in software, where these bit tricks are much cheaper
// than the operations they stand for: the exponent of a double's upper
// word, and dividing by 2^28 by lowering it
typedef union { double d; __QWORD__ q; } dbits_t;

static int big(double dval, int e){

	dbits_t 	b;

	b.d = dval;

	return (int)((b.q >> 52)&0x7ff) >= 1023+e;
}

// 4.28 fixed point to double, exactly as v*(1.0/268435456.0) would
static double fix28(int v){

	dbits_t 	b;

	b.d = (double)v;
	if(v) b.q -= (__QWORD__)28 << 52;

	return b.d;
}

// radians to 1/2^24 turns. fmod() is exact, so far-off angles still agree everywhere
static int turns(double dval){

	if(big(dval, 9)) dval = fmod(dval, 6.283185307179586);

	return (int)(dval*2670176.857720436);
}

double dRMath_sin(double dval){ return fix28(dRMath_sin_i(turns(dval))); }
double dRMath_cos(double dval){ return fix28(dRMath_cos_i(turns(dval))); }

// degrees, which is what the race code has: it saves the two soft-float
// operations, one a division, that turned them into radians
static int turns_deg(double deg){

	if(big(deg, 14)) deg = fmod(deg, 360.0);

	return (int)(deg*46603.37777777778);
}

double dRMath_sin_deg(double deg){ return fix28(dRMath_sin_i(turns_deg(deg))); }
double dRMath_cos_deg(double deg){ return fix28(dRMath_cos_i(turns_deg(deg))); }

// degrees to 1/2^24 turns, from a float: 360 degrees still fit its 24 bits
int dRMath_turns_degf(float deg){

	if((deg >= 32768.0f)||(deg <= -32768.0f)) deg = fmodf(deg, 360.0f);

	return (int)(deg*46603.377f);
}

#if !defined(DR_INLINE_MATH)
double dRMath_sqrt(double dval){ return sqrt(dval); }
double dRMath_ceil(double dval){ return ceil(dval); }
double dRMath_floor(double dval){ return floor(dval); }

double dRMath_abs(double dval){ return (dval < 0.0)?(-1.0*dval):dval; }

int dRMath_sum2squares_i(int val1, int val2){ return val1*val1+val2*val2; }
int dRMath_square_i(int val){ return val*val; }
int dRMath_abs_i(int val){ return (val<0)?(0-val):val; }
int dRMath_min_i(int val1, int val2){ return (val1 < val2)?val1:val2; }
int dRMath_max_i(int val1, int val2){ return (val1 < val2)?val2:val1; }
int dRMath_absneg_i(int val){ return (val<0)?(0-val):0; }
int dRMath_abspos_i(int val){ return (val>0)?val:0; }


double dRMath_sum2squares(double dval1, double dval2){ return dval1*dval1+dval2*dval2; }
double dRMath_magnitude2(double a, double b){ return dRMath_sqrt(dRMath_sum2squares(a, b)); }
int dRMath_magnitude2_i(int a, int b){ return (int)dRMath_sqrt((double)dRMath_sum2squares_i(a, b)); }

double dRMath_sum3squares(double dval1, double dval2, double dval3){ return dval1*dval1+dval2*dval2+dval3*dval3; }
double dRMath_magnitude3(double a, double b, double c){ return dRMath_sqrt(dRMath_sum3squares(a, b, c)); }
#endif // DR_INLINE_MATH
