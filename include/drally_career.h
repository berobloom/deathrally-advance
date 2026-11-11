#ifndef __DRALLY_CAREER_H
#define __DRALLY_CAREER_H

#include "drally.h"

// the career's state between races, see src/career/career.c

// DR_CAREER_NODRAW: the menus' functions that career.c calls leave out
// their drawing, for the GBA and tools/replay, which have no menu screen
#if defined(DR_CAREER_NODRAW)
#define CAREER_DRAW 		0
#else
#define CAREER_DRAW 		1
#endif

// career_signup_check()'s answers
#define CAREER_FULL 		1	// the race has its 4
#define CAREER_NEEDS_CAR 	2	// the first time: a better car first, said once

void career_new(const char * name, int face, int difficulty);
int career_leading(void);
void career_signup_open(void);
int career_signup_check(int race);
void career_signup(int race);
int career_signup_full(void);
void career_signup_rest(void);
void career_signup_order(void);
void career_race_track(__BYTE__ grid[4]);
void career_race_fill(int NumCars, const __BYTE__ grid[4]);
void career_race_after(int NumCars, const __BYTE__ grid[4]);
int career_leader_round(void);
void career_lap_record(void);
void career_leader_check(void);
int career_results(int A1);
void career_adversary_decline(void);
void career_adversary_declined(void);
void career_adversary_accept(void);

#endif // __DRALLY_CAREER_H
