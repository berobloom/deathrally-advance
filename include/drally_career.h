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

// the shop's items, as shop_main() numbers them in D(___1a1ec4h)
#define CAREER_SHOP_CAR 	0	// the one on show, D(___1a1ee4h)
#define CAREER_SHOP_ENGINE 	1
#define CAREER_SHOP_TIRES 	2
#define CAREER_SHOP_ARMOR 	3
#define CAREER_SHOP_REPAIR 	4
#define CAREER_SHOP_NONE 	(-0x7fffffff)	// career_shop_price(): none to buy

int career_shop_trade_in(void);
int career_shop_level(int item);
int career_shop_most(int item);
int career_shop_price(int item);
void career_shop_buy(int item);
int career_shop_can_race(void);
int career_shop_broke(void);

// the underground market's items, as underground_main() numbers them in
// D(___1a1ef0h): the loan shark first
#define CAREER_MARKET_LOAN 	0
#define CAREER_MARKET_MINES 	1
#define CAREER_MARKET_SPIKES 	2
#define CAREER_MARKET_FUEL 	3
#define CAREER_MARKET_SABOTAGE 	4

void career_market_open(void);
int career_market_state(int item);
int career_market_price(int item);
void career_market_buy(int item);
int career_loan_amount(void);
void career_loan_take(void);
int career_loan_owed(void);
void career_loan_pay(void);
int career_loan_collect(int clear);
int career_loan_due(void);
int career_signup_sabotage(__DWORD__ seed);

// the random events and what else the shop tells when you come back, as
// shop_main() numbers them
#define CAREER_EVENT_NONE 		0
#define CAREER_EVENT_WELCOME 		1	// the shop's, the first time
#define CAREER_EVENT_STREAK 		2	// three wins in a row
#define CAREER_EVENT_INCONCEIVABLE 	3	// no damage in a race with weapons
#define CAREER_EVENT_REAPER 		4	// all the others wrecked
#define CAREER_EVENT_STEROIDS 		5	// the offer's outcome
#define CAREER_EVENT_HITMAN 		6	// the offer's outcome
#define CAREER_EVENT_LOAN 		7	// the loan shark collects
#define CAREER_EVENT_LAPPED 		9
#define CAREER_EVENT_TOO_SLOW 		10	// last

int career_event_next(void);
int career_event_apply(int ev);
void career_event_clear(int ev);
void career_shop_continue(void);
int career_market_welcome(void);
int career_offer(int * pay, int * target);
void career_offer_accept(int ev, int target);

#endif // __DRALLY_CAREER_H
