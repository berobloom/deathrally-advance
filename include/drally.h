#ifndef __DRALLY_H
#define __DRALLY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <SDL2/SDL.h>

#include "types.h"

// where the GBA (gba/replay) wants code and data that aren't in the
// default places: DR_HOT for the race code's innermost loops, as ARM code
// in IWRAM, DR_COLD for big race state it seldom reads, in EWRAM to leave
// IWRAM to the rest. Nothing on the desktop
#if !defined(DR_HOT)
#define DR_HOT
#endif
#if !defined(DR_COLD)
#define DR_COLD
#endif


typedef void (*void_cb)(void);

#define S(s, type, offset)     (*(type *)((__POINTER__)(s)+(offset)))

// Q/D/W/P read and write at any byte address, as the original x86 code
// did. Through a packed, may_alias struct they stay lvalues, and CPUs that
// can't load unaligned words (the GBA's ARM7) get byte loads instead.
//
// DR_ALIGNED (the GBA build) has them load whole words instead, which takes
// a tick of the race from 192,900 to 182,100 cycles. An unaligned address
// would then read the wrong bytes without any error, so DR_ALIGNED_CHECK
// tests each one and counts those that aren't aligned: dr_misaligned, the
// first at dr_misaligned_file and dr_misaligned_line.
#if defined(DR_ALIGNED)
typedef __QWORD__ __attribute__((may_alias)) __aqword__;
typedef __DWORD__ __attribute__((may_alias)) __adword__;
typedef __WORD__ __attribute__((may_alias)) __aword__;
typedef __POINTER__ __attribute__((may_alias)) __apointer__;
#if defined(DR_ALIGNED_CHECK)
extern unsigned dr_misaligned, dr_misaligned_line;
extern const char * dr_misaligned_file;
static inline void * dr_aligned(void * p, unsigned mask, const char * file, unsigned line){

	if(((unsigned long)p&mask)&&(dr_misaligned++ == 0)){

		dr_misaligned_file = file;
		dr_misaligned_line = line;
	}

	return p;
}
#define DR_AL(p, mask)	dr_aligned((void *)(p), mask, __FILE__, __LINE__)
#else
#define DR_AL(p, mask)	((void *)(p))
#endif // DR_ALIGNED_CHECK
#define Q(q)    (*(__aqword__ *)DR_AL(q, 3))
#define D(d)	(*(__adword__ *)DR_AL(d, 3))
#define W(w)	(*(__aword__ *)DR_AL(w, 1))
#define P(p)    (*(__apointer__ *)DR_AL(p, 3))
#elif defined(__GNUC__)
typedef struct { __QWORD__ v; } __attribute__((packed, may_alias)) __uqword__;
typedef struct { __DWORD__ v; } __attribute__((packed, may_alias)) __udword__;
typedef struct { __WORD__ v; } __attribute__((packed, may_alias)) __uword__;
typedef struct { __POINTER__ v; } __attribute__((packed, may_alias)) __upointer__;
#define Q(q)    (((__uqword__ *)(q))->v)
#define D(d)	(((__udword__ *)(d))->v)
#define W(w)	(((__uword__ *)(w))->v)
#define P(p)    (((__upointer__ *)(p))->v)
#else
#define Q(q)    (*(__QWORD__ *)(q))
#define D(d)	(*(__DWORD__ *)(d))
#define W(w)	(*(__WORD__ *)(w))
#define P(p)    (*(__POINTER__ *)(p))
#endif
#define B(b)	(*(__BYTE__ *)(b))

#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
#define X(r)	((__WORD__ *)&r)[1]
#define H(r)	((__BYTE__ *)&r)[2]
#define L(r)	((__BYTE__ *)&r)[3]
#else
#define X(r)	((__WORD__ *)&r)[0]
#define H(r)	((__BYTE__ *)&r)[1]
#define L(r)	((__BYTE__ *)&r)[0]
#endif

FILE * strupr_fopen(const char * file_name, const char * mode);

#define sES 0
#define sCS 2
#define sSS 4
#define sDS 6
#define sFS 8
#define sGS 0xa

#define __ES(base)  W(base + sES)
#define __CS(base)  W(base + sCS)
#define __SS(base)  W(base + sSS)
#define __DS(base)  W(base + sDS)
#define __FS(base)  W(base + sFS)
#define __GS(base)  W(base + sGS)

#define rEAX    0
#define rEBX    4
#define rECX    8
#define rEDX    0xc
#define rESI    0x10
#define rEDI    0x14
#define rCF     0x18

#define __EAX(base) D(base + rEAX)
#define __AX(base)  W(base + rEAX)
#define __AL(base)  B(base + rEAX)
#define __EBX(base) D(base + rEBX)
#define __BX(base)  W(base + rEBX)
#define __BL(base)  B(base + rEBX)
#define __ECX(base) D(base + rECX)
#define __CX(base)  W(base + rECX)
#define __CL(base)  B(base + rECX)
#define __EDX(base) D(base + rEDX)
#define __DX(base)  W(base + rEDX)
#define __DL(base)  B(base + rEDX)
#define __ESI(base) D(base + rESI)
#define __SI(base)  W(base + rESI)
#define __EDI(base) D(base + rEDI)
#define __DI(base)  W(base + rEDI)
#define __CF(base)  D(base + rCF)

void ___idiv32(__POINTER__ eax, __POINTER__ edx, int dv);
void ___idiv16(__POINTER__ ax, __POINTER__ dx, short dv);
void ___imul32(__POINTER__ eax, __POINTER__ edx, int mp);



typedef double  m64fp;
typedef float   m32fp;

#define F32(f)  (*(m32fp *)(f))
#define F64(f)  (*(m64fp *)(f))

#define ST(n)       (*fpu_st(n))
#define FPUSH(d)    fpu_push((double)(d))
#define FPOP        fpu_pop
double * fpu_st(int);
void fpu_push(double);
double fpu_pop(void);

#define dR_PI   3.141592653589793238462643383279502884197169399375105820974

double create_double(int LSB, int, int, int, int, int, int, int MSB);

#include "types.h"


void incCounter(int n);
void decCounter(int n);

// inline: the race code reads counter 4, its slot in the cars' controls,
// a few hundred times a tick
	extern __BYTE__ ___243ca4h[];
	extern __BYTE__ SUPERGLOBAL___243898h[];
	extern __BYTE__ SUPERGLOBAL___243894h[];
	extern __BYTE__ ___243ca8h[];
	extern __BYTE__ ___243874h[];
	extern __BYTE__ ___2438d0h[];

static inline __DWORD__ getCounter(int n){

	if(n == 1) return D(___243ca4h);
	if(n == 2) return D(SUPERGLOBAL___243898h);
	if(n == 3) return D(SUPERGLOBAL___243894h);
	if(n == 4) return D(___243ca8h);
	if(n == 5) return D(___243874h);
	if(n == 6) return D(___2438d0h);

	return 0;
}
void resetCounter(int n);
void setCounter(int n, __DWORD__ val);


#define read_b(p)       (*(__BYTE__*)(p))
#define write_b(p,b)    (*(__BYTE__*)(p) = (b))   
void switch_b(__POINTER__, __POINTER__);






#define ___24e4d0h          CONNECTION_TYPE
#define ___185a1ch          ChatMaximized
#define ___632c4h           swap16
#define ___199fbch          MP_ERROR
#define ___243cd0h          SUPERGLOBAL___243cd0h
#define ___243898h          SUPERGLOBAL___243898h
#define ___243894h          SUPERGLOBAL___243894h
#define ___243cach          SUPERGLOBAL___243cach
#define ___243ce8h          MY_CAR_IDX
#define ___243cf4h          NUM_OF_CARS
#define ___243d28h          TRX_WIDTH
#define ___243d30h          TRX_WIDTH_QTR
#define ___243d04h          TRX_HEIGHT_QTR
#define ___243d2ch          TRX_HEIGHT
#define ___243d58h          TRX_IMA
#define ___243d60h          TRX_MAS
#define ___243d78h          TRX_VAI
#define ___243d54h          TRX_LR1
#define ___240a48h          VARJO_TAB
#define ___243078h          TRX_SKI_TAB
#define ___242d78h          TRX_BLO_TAB
#define ___241c78h          TRX_LIT_TAB
#define ___243dd0h          TRX_BPA
#define ___242578h          TRX_DRV_DAT
#define ___242178h          TRX_OHI_DAT
#define ___243d8ch          RACE_MINES1A_BPK
#define ___243d7ch          RACE_BLOWI_BPK
#define ___243298h          RACE_FLAMEX_BPK
#define ___243d88h          RACE_SHOTS_BPK
#define ___243d38h          RACE_OBSTACLE_BPK
#define ___243d40h          RACE_PEDESTR_BPK
#define ___243288h          RACE_SPLATX_BPK
#define ___243290h          RACE_ROCKETX_BPK
#define ___243d68h          RACE_BURN1A_BPK
#define ___243d64h          RACE_SMOKE_BPK
#define ___243d50h          GEN_FLA_BPK
#define ___243d6ch          GEN_LAM_BPK
#define ___243d4ch          GEN_MES_BPK
#define ___243d70h          CPY_GEN_MES_BPK
#define ___243d48h          DAMSLID_BPK
#define ___243d84h          RASTI1_BPK
#define ___243d3ch          GOALNUM2_BPK
#define ___243d34h          SIDEBOM1_BPK
#define ___1de920h          X___1de920h
#define ___1ded20h          Y___1ded20h
#define ___196dc8h          TRX_VIEWPORT_TL_X
#define ___196dcch          TRX_VIEWPORT_TL_Y
#define ___243c6ch          X1___243c6ch
#define ___243c68h          X2___243c68h
#define ___243c64h          X3___243c64h
#define ___243c78h          Y1___243c78h
#define ___243c70h          Y3___243c70h
#define ___243c74h          Y2___243c74h
#define ___196d8ch          CURRENT_VIEWPORT_CENTER_X
#define ___196d94h          CURRENT_VIEWPORT_CENTER_Y
#define ___196d98h          CURRENT_VIEWPORT_X
#define ___243c94h          NUM_OF_TEXTURES
#define ___243c88h          NUM_OF_OBJECTS
#define ___196d88h          CURRENT_VIEWPORT_W
#define ___196d90h          CURRENT_VIEWPORT_H
#define ___243c8ch          X___243c8ch
#define ___243c90h          Y___243c90h
#define ___243cc0h          X_Inc
#define ___243cc4h          Y_Inc
#define ___243cc8h          X_IncOld
#define ___243ccch          Y_IncOld
#define ___243d80h          BACKBUFFER
#define ___1a1094h          LAP_PREVIOUS_MIN
#define ___1a109ch          LAP_PREVIOUS_SEC
#define ___1a102ch          LAP_PREVIOUS_100
#define ___1a1034h          LAP_BEST_MIN
#define ___1a108ch          LAP_BEST_SEC
#define ___1a0ff4h          LAP_BEST_100
#define ___2438bch          LAP_RECORD_MIN
#define ___2438b8h          LAP_RECORD_SEC
#define ___2438c0h          LAP_RECORD_100
#define ___1a1134h          NUM_OF_LAPS
#define ___185a54h          MP_MONEY

#endif // __DRALLY_H
