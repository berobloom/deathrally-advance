// Replays a race recorded by the desktop game (make ticklog) through the
// game's own race_logic(), built for the GBA, and times every frame.
//
// The track maps and car images stay in ROM, as they will have to in the
// port. Each frame applies what the desktop saw change before its logic ran
// (input, the timer, rendering), runs race_logic(), and compares a hash of
// the globals with the desktop's, so the GBA build is checked to compute
// the same race. Results go to mGBA's debug log and the screen.

#include <gba_base.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_timers.h>
#include <gba_console.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "drally.h"
#include "race_replay.h"

extern const __DWORD__ ticklog[];

void race_logic(void);
__DWORD__ getCounter(int n);

#define REG_WAITCNT		(*(vu16 *)0x04000204)
#define REG_DEBUG_ENABLE	(*(vu16 *)0x04FFF780)
#define REG_DEBUG_FLAGS		(*(vu16 *)0x04FFF700)
#define REG_DEBUG_STRING	((char *)0x04FFF600)

#if !defined(REPLAY_FRAMES)
#define REPLAY_FRAMES		~0u		// make FRAMES=n stops after n frames
#endif
#define FRAME_CYCLES		280896		// one 59.73 Hz frame of the 16.78 MHz CPU
#define MAX_TICKS		4
// unmapped on the GBA: writes go nowhere, also on real hardware, where a
// write to ROM could reach a flash cartridge's command registers
#define NOWHERE			((__POINTER__)0x00010000)

#if defined(PROFILE)
// make PROFILE=1 wraps race_logic()'s parts (-Wl,--wrap) to time each one;
// time spent in sin() and cos() also counts toward their callers
#define PROFILED(PX)	PX(race___4b62ch) PX(race___4c434h) PX(race___4dcach) PX(race___514d8h) \
	PX(race___52ac4h) PX(race___526ach) PX(race___4ff50h) PX(race___54668h) PX(race___55ae0h) \
	PX(race___56170h) PX(race___563ach) PX(race___5209ch) PX(race___4c21ch) PX(sin) PX(cos)
enum {
#define PX(f)	PROF_##f,
	PROFILED(PX)
#undef PX
	PROF_N
};
static const char * prof_names[PROF_N] = {
#define PX(f)	#f,
	PROFILED(PX)
#undef PX
};
static unsigned long long 	prof_cycles[PROF_N];
static int 			in_libm;	// soft-float calls inside sin() and cos() aren't counted
static unsigned 		prof_calls[PROF_N];
static u32 cycles(void);
#define PROF_WRAP(ret, f, decl, args) \
	ret __real_##f decl; \
	ret __wrap_##f decl { u32 t = cycles(); ret r = __real_##f args; in_libm = 0; prof_cycles[PROF_##f] += cycles()-t; prof_calls[PROF_##f]++; return r; }
#define PROF_WRAP_VOID(f, decl, args) \
	void __real_##f decl; \
	void __wrap_##f decl { u32 t = cycles(); __real_##f args; prof_cycles[PROF_##f] += cycles()-t; prof_calls[PROF_##f]++; }
PROF_WRAP_VOID(race___4b62ch, (int n), (n))
PROF_WRAP_VOID(race___4c434h, (void), ())
PROF_WRAP_VOID(race___4dcach, (void), ())
PROF_WRAP_VOID(race___514d8h, (void), ())
PROF_WRAP_VOID(race___52ac4h, (void), ())
PROF_WRAP_VOID(race___526ach, (void), ())
PROF_WRAP_VOID(race___4ff50h, (void), ())
PROF_WRAP_VOID(race___54668h, (void), ())
PROF_WRAP_VOID(race___55ae0h, (void), ())
PROF_WRAP_VOID(race___56170h, (void), ())
PROF_WRAP_VOID(race___563ach, (void), ())
PROF_WRAP_VOID(race___5209ch, (void), ())
PROF_WRAP_VOID(race___4c21ch, (void), ())
PROF_WRAP(double, sin, (double x), ((in_libm++, x)))
PROF_WRAP(double, cos, (double x), ((in_libm++, x)))

// and counts the soft-float calls: ret, name, parameters, arguments
#define FLOATOPS(F) \
	F(double, __aeabi_dadd, (double a, double b), (a, b)) F(double, __aeabi_dsub, (double a, double b), (a, b)) \
	F(double, __aeabi_dmul, (double a, double b), (a, b)) F(double, __aeabi_ddiv, (double a, double b), (a, b)) \
	F(float, __aeabi_fadd, (float a, float b), (a, b)) F(float, __aeabi_fsub, (float a, float b), (a, b)) \
	F(float, __aeabi_fmul, (float a, float b), (a, b)) F(float, __aeabi_fdiv, (float a, float b), (a, b)) \
	F(double, __aeabi_f2d, (float a), (a)) F(float, __aeabi_d2f, (double a), (a)) \
	F(double, __aeabi_i2d, (int a), (a)) F(double, __aeabi_ui2d, (unsigned a), (a)) \
	F(int, __aeabi_d2iz, (double a), (a)) F(unsigned, __aeabi_d2uiz, (double a), (a)) \
	F(int, __aeabi_f2iz, (float a), (a)) F(float, __aeabi_i2f, (int a), (a)) \
	F(int, __aeabi_dcmplt, (double a, double b), (a, b)) F(int, __aeabi_dcmpgt, (double a, double b), (a, b)) \
	F(int, __aeabi_dcmpeq, (double a, double b), (a, b)) F(int, __aeabi_fcmpgt, (float a, float b), (a, b)) \
	F(int, __aeabi_fcmpge, (float a, float b), (a, b)) F(int, __aeabi_fcmple, (float a, float b), (a, b)) \
	F(int, __aeabi_fcmplt, (float a, float b), (a, b))
enum {
#define PF(ret, f, decl, args)	FOP_##f,
	FLOATOPS(PF)
#undef PF
	FOP_N
};
static const char * fop_names[FOP_N] = {
#define PF(ret, f, decl, args)	#f,
	FLOATOPS(PF)
#undef PF
};
static unsigned 	fop_calls[FOP_N];
static unsigned long long 	fop_cycles;	// in all of them, with the wrappers' own cost
#define PF(ret, f, decl, args) \
	ret __real_##f decl; \
	ret __wrap_##f decl { if(in_libm) return __real_##f args; u32 t = cycles(); ret r = __real_##f args; fop_cycles += cycles()-t; fop_calls[FOP_##f]++; return r; }
FLOATOPS(PF)
#undef PF
#endif // PROFILE

// sound isn't measured here
void dRally_Sound_pushEffect(__BYTE__ channel, __BYTE__ n, __DWORD__ unk, __DWORD__ a0, __DWORD__ a1, __DWORD__ a2){}
void dRally_Sound_freeEffectChannel(__BYTE__ ch_num){}
void dRally_Sound_adjustEffect(__BYTE__ channel, __DWORD__ vol, __DWORD__ freq, __DWORD__ balance){}

EWRAM_BSS static __BYTE__ flat[0x4000] __attribute__((aligned(4)));
EWRAM_BSS static __BYTE__ obstacles[0x800];

static void say(const char * fmt, ...){

	char 		s[128];
	va_list 	ap;

	va_start(ap, fmt);
	vsnprintf(s, sizeof(s), fmt, ap);
	va_end(ap);

	if(REG_DEBUG_ENABLE == 0x1dea){

		strncpy(REG_DEBUG_STRING, s, 255);
		REG_DEBUG_FLAGS = 3 | 0x100;	// info level, print
	}

	// and on screen, for real hardware
	printf("%s\n", s);
}

static u32 cycles(void){

	u16 	hi, lo;

	do {
		hi = REG_TM3CNT_L;
		lo = REG_TM2CNT_L;
	} while(hi != REG_TM3CNT_L);

	return ((u32)hi << 16) | lo;
}

// percent of a frame, one decimal
#define PCT(c)		(int)((c)*100ULL/FRAME_CYCLES), (int)((c)*1000ULL/FRAME_CYCLES%10)

int main(void){

	const __DWORD__ * 	p = ticklog;
	unsigned 		i, n, nruns, frames, mismatches, first_bad;
	u32 			t, tmax[MAX_TICKS+1], fcount[MAX_TICKS+1], ticks, worst;
	unsigned long long 	tsum[MAX_TICKS+1], total;

	irqInit();
	irqEnable(IRQ_VBLANK);
	REG_WAITCNT = 0x4007;		// ROM at 3/2 wait states with prefetch, works on flash carts
	REG_DEBUG_ENABLE = 0xc0de;
	consoleDemoInit();

	if((p[0] != RACE_REPLAY_MAGIC)||(p[1] != RACE_REPLAY_VERSION)){ say("no TICKLOG.BIN in this ROM"); while(1) VBlankIntrWait(); }

	n = p[2];
	if((n != race_replay_size())||(p[3] != race_replay_count())||(n > sizeof(flat))){

		say("log has %u bytes in %u globals, this build %u in %u", n, p[3], race_replay_size(), race_replay_count());
		while(1) VBlankIntrWait();
	}
	for(i = 0; i < p[3]; i++){

		if(p[4+i] != race_replay_var_size(i)){ say("global %u: %u bytes in the log, %u here", i, p[4+i], race_replay_var_size(i)); while(1) VBlankIntrWait(); }
	}
	p += 4+p[3];

	for(i = 0; i < RACE_REPLAY_NBUFS; i++){

		__DWORD__ 	size = *p++;

		*race_replay_buf(i) = (__POINTER__)p;
		p += (size+3)/4;
	}
	*race_replay_buf(3) = NOWHERE;
	memcpy(obstacles, *race_replay_buf(5), sizeof(obstacles));
	*race_replay_buf(5) = obstacles;

	memcpy(flat, p, n);
	p += (n+3)/4;

	memset(tsum, 0, sizeof(tsum)); memset(tmax, 0, sizeof(tmax)); memset(fcount, 0, sizeof(fcount));
	frames = mismatches = 0; first_bad = ~0u; total = ticks = worst = 0;

	REG_TM2CNT_H = 0; REG_TM3CNT_H = 0;
	REG_TM2CNT_L = 0; REG_TM3CNT_L = 0;
	REG_TM3CNT_H = TIMER_START | TIMER_COUNT;
	REG_TM2CNT_H = TIMER_START;

	say("replaying %u bytes of race state", n);

	while(((nruns = *p++) != RACE_REPLAY_END)&&(frames < REPLAY_FRAMES)){

		unsigned 	k;

		for(k = 0; k < nruns; k++){

			__DWORD__ 	off = *p++, len = *p++;

			memcpy(flat+off, p, len);
			p += (len+3)/4;
		}
		race_replay_scatter(flat);

		k = getCounter(2);	// the timer ticks this frame's logic will run
		if(k > MAX_TICKS) k = MAX_TICKS;

		t = cycles();
		race_logic();
		t = cycles()-t;

		race_replay_gather(flat);
		if(race_replay_hash(flat) != *p++){

			if(first_bad == ~0u){

				// the globals as this build left them, to compare with the
				// desktop's TICKPOST.BIN (DR_TICKLOG_POST=1 make ticklog)
				first_bad = frames;
				say("first mismatch in frame %u, globals after it:", frames);
				for(i = 0; i < n; i += 32){

					char 	hex[72];
					int 	j;

					for(j = 0; j < 32; j++) snprintf(hex+2*j, 3, "%02x", flat[i+j]);
					if(REG_DEBUG_ENABLE == 0x1dea){ snprintf(REG_DEBUG_STRING, 255, "POST %05x %s", i, hex); REG_DEBUG_FLAGS = 3 | 0x100; }
				}
			}
			mismatches++;
		}

		tsum[k] += t; fcount[k]++;
		if(t > tmax[k]) tmax[k] = t;
		total += t; ticks += k; frames++;
		if(t > worst) worst = t;
		if(frames%1000 == 0) say("frame %u: %u ticks, %u bad", frames, ticks, mismatches);
	}

	say("%u frames, %u ticks", frames, ticks);
	if(mismatches) say("MISMATCH in %u frames, first %u", mismatches, first_bad);
	else say("all hashes match the desktop");
	for(i = 0; i <= MAX_TICKS; i++){

		if(fcount[i]) say("%u ticks: %u frames, avg %u.%u%% max %u.%u%%", i, fcount[i], PCT(tsum[i]/fcount[i]), PCT(tmax[i]));
	}
	t = ticks ? total/ticks : 0;
	say("per tick: %u cycles = %u.%u%% of a frame", t, PCT(t));
	say("70 ticks/s at 60 fps: %u.%u%% of the CPU", PCT((unsigned long long)t*70/60));
	say("worst frame %u.%u%%", PCT(worst));
#if defined(PROFILE)
	for(i = 0; i < PROF_N; i++){

		if(prof_calls[i]) say("%-14s %7u calls %6u cycles each, %u.%u%% a tick", prof_names[i], prof_calls[i],
			(u32)(prof_cycles[i]/prof_calls[i]), PCT(ticks ? prof_cycles[i]/ticks : 0));
	}
	for(i = 0, t = 0; i < FOP_N; i++){

		t += fop_calls[i];
		if(fop_calls[i]) say("%-16s %6u a tick", fop_names[i]+7, ticks ? fop_calls[i]/ticks : 0);
	}
	say("soft-float calls: %u a tick, %u cycles", ticks ? t/ticks : 0, ticks ? (u32)(fop_cycles/ticks) : 0);
#endif // PROFILE

	while(1) VBlankIntrWait();
}
