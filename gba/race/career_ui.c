// The career on the GBA: the game's own (src/career/career.c, the state
// its menus keep between races) behind text screens of the GBA's own, until
// the menus' pictures come. A new game, then rounds of three races: sign up
// for one, race it, the round's results, the standings, until you lead on
// points and beat the Adversary.
//
// RACE_CFLAGS="-DRACE_BENCH -DCAREER_BENCH=n -DRACE_SEED=s" plays n rounds by itself,
// the AI driving your car, and logs a hash of the racers after each, to
// compare with tools/replay --career.

#include <gba_base.h>
#include <gba_input.h>
#include <gba_systemcalls.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "drally.h"
#include "drally_structs_fixed.h"
#include "drally_career.h"
#include "gba.h"

	extern __BYTE__ ___1a01e0h[];
	extern __BYTE__ ___1a1ef8h[];
	extern __BYTE__ ___1a1028h[];
	extern __BYTE__ ___1a103ch[];
	extern __BYTE__ ___1a1f64h[];
	extern __BYTE__ ___1a0ef8h[];
	extern __BYTE__ ___1de7d0h[];
	extern __BYTE__ ___196aech[];
	extern __BYTE__ ___196af0h[];
	extern __BYTE__ ___196ae8h[];
	extern char ___19bd64h[16];
	extern int NUM_OF_LAPS;
	extern __POINTER__ ___1a0fe0h;
	extern __POINTER__ ___1a0fb8h;
	extern const char name_list[0x14][0xb];
	extern const char ___106cbh[19][4];

extern const u8 carcol_pal[], copper_pal[];	// baked from MENU.BPA, see assets.S
void srand_watcom106(__DWORD__ seed);
void ___3079ch_cdecl(__DWORD__);

#define RACERS			0x14
#define ME			(D(___1a1ef8h))
#define COLS			30
#define ROWS			20

static racer_t * racer(int n){ return (racer_t *)___1a01e0h+n; }

static const char * const car_names[7] = { "VAGABOND", "DERVISH", "SENTINEL", "SHRIEKER", "WRAITH", "DELIVERATOR", "ADVERSARY" };
static const char * const difficulty_names[3] = { "SPEED MAKES ME DIZZY", "I LIVE TO RIDE", "PETROL IN MY VEINS" };
static const char * const race_names[4] = { "EASY", "MEDIUM", "HARD", "ADVERSARY" };
static const int prizes[3] = { 750, 3000, 12000 };

#if defined(CAREER_BENCH)
static int bench_round;
#endif

// --- text ----------------------------------------------------------------------

static void print(int col, int row, const char * fmt, ...) __attribute__((format(printf, 3, 4)));
static void print(int col, int row, const char * fmt, ...){

	char 	line[40];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(line, sizeof(line), fmt, ap);
	va_end(ap);
	gba_print(col, row, line);
}

static void center(int row, const char * s){

	gba_print((COLS-(int)strlen(s))/2, row, s);
}

// a row of a menu, an arrow at the one chosen
static void item(int row, int chosen, const char * s){

	gba_print(2, row, (row == chosen) ? ">" : " ");
	gba_print(4, row, s);
}

// until A or Start, or B if it may: 1 for B
static int wait_key(int b){

#if defined(CAREER_BENCH)
	(void)b;
	gba_frame();
	return 0;
#else
	for(;;){

		u16 	k = gba_frame();

		if(k&(KEY_A|KEY_START)) return 0;
		if(b&&(k&KEY_B)) return 1;
	}
#endif
}

// a message in the middle of a cleared screen, until a key
static void message(const char * a, const char * b){

	gba_text_screen();
	center(8, a);
	if(b) center(10, b);
	center(17, "PRESS A");
	wait_key(0);
}

// the track a race of the round is on: TR1 to TR9, R reversed
static const char * track_name(int race, char * buf){

	int 	k = (race == 3) ? 18 : B(___1a1f64h+race);

	sprintf(buf, "%s%s", ___106cbh[k], (k > 8)&&(k < 18) ? " REV" : "");
	return buf;
}

#if defined(CAREER_BENCH)
// a hash of the racers, as tools/replay --career logs it
static u32 racers_hash(void){

	const u8 * 	p = ___1a01e0h;
	u32 		h = 0x811c9dc5;
	unsigned 	n;

	for(n = 0; n < RACERS*sizeof(racer_t); n++) h = (h^p[n])*0x01000193;
	return h;
}
#endif

// --- the title and a new game ----------------------------------------------------

int gba_title(void){

	int 	row = 9;

	gba_text_screen();
	center(4, "DEATH RALLY ADVANCE");
	center(17, "PRESS A");
	for(;;){

		u16 	k;

		item(9, row, "CAREER");
		item(11, row, "QUICK RACE");
		k = gba_frame();
		if(k&(KEY_UP|KEY_DOWN)) row = 20-row;
		if(k&(KEY_A|KEY_START)) return row != 9;
	}
}

#define NAME_LEN		9
static const char name_chars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.'";

// your name, face and difficulty, as ___3ab5ch_cdecl() asks for them. 0 for B
static int new_game(void){

	static char 	name[NAME_LEN+1] = "RACER";
	static int 	face = 0, difficulty = 1;
	int 		row = 0, edit = 0, cur = 0, n;
	char 		buf[32];

#if defined(CAREER_BENCH)
	career_new(name, face, difficulty);
	return 1;
#endif
	gba_text_screen();
	center(1, "NEW GAME");
	for(;;){

		u16 	k;

		for(n = strlen(name); n < NAME_LEN; n++) name[n] = ' ';
		name[NAME_LEN] = 0;
		gba_print(4, 4, "NAME");
		gba_print(11, 4, name);
		for(n = 0; n < NAME_LEN; n++) gba_print(11+n, 5, (edit&&(n == cur)) ? "-" : " ");
		gba_print(4, 7, "FACE");
		print(11, 7, "%-11s", name_list[face]);
		gba_print(4, 10, "DIFFICULTY");
		print(4, 11, "%-22s", difficulty_names[difficulty]);
		gba_print(4, 14, "START");
		gba_print(2, 4, (row == 0) ? ">" : " ");
		gba_print(2, 7, (row == 1) ? ">" : " ");
		gba_print(2, 10, (row == 2) ? ">" : " ");
		gba_print(2, 14, (row == 3) ? ">" : " ");
		gba_print(2, 18, edit ? "UP/DOWN LETTER, A DONE " : "A CHOOSE, B BACK       ");

		k = gba_frame();
		if(edit){

			const char * 	c = strchr(name_chars, name[cur]);
			int 		i = c ? c-name_chars : 0, m = sizeof(name_chars)-1;

			if(k&KEY_UP) name[cur] = name_chars[(i+1)%m];
			if(k&KEY_DOWN) name[cur] = name_chars[(i+m-1)%m];
			if((k&KEY_LEFT)&&(cur > 0)) cur--;
			if((k&KEY_RIGHT)&&(cur < NAME_LEN-1)) cur++;
			if(k&(KEY_A|KEY_B|KEY_START)) edit = 0;
			continue;
		}
		if(k&KEY_B) return 0;
		if(k&KEY_UP) row = (row+3)%4;
		if(k&KEY_DOWN) row = (row+1)%4;
		n = (k&KEY_RIGHT) ? 1 : (k&KEY_LEFT) ? -1 : 0;
		if(row == 1) face = (face+RACERS+n)%RACERS;
		if(row == 2) difficulty = (difficulty+3+n)%3;
		if(k&(KEY_A|KEY_START)){

			if(row == 0){ edit = 1; continue; }
			if(row != 3){ row++; continue; }
			// the name without its trailing spaces
			strcpy(buf, name);
			for(n = strlen(buf); (n > 0)&&(buf[n-1] == ' '); n--) buf[n-1] = 0;
			if(!buf[0]) strcpy(buf, "RACER");
			career_new(buf, face, difficulty);
			return 1;
		}
	}
}

// --- the standings -----------------------------------------------------------------

// all 20 by rank, two pages of 10, with what each scored in the round if gained
static void standings(const int * gained){

	int 	page, n;

	for(page = 0; page < 2; page++){

		gba_text_screen();
		center(1, "STANDINGS");
		print(1, 3, "RANK NAME        POINTS");
		for(n = 10*page; n < 10*page+10; n++){

			racer_t * 	r = racer(n);
			int 		row = 5+n%10;

			print(1, row, "%2d. %-11s %3d", r->rank, r->name, r->points);
			if(gained&&gained[r->face]) print(24, row, "+%d", gained[r->face]);
			if(n == ME) gba_print(0, row, ">");
		}
		center(18, page ? "PRESS A" : "PRESS A FOR MORE");
		wait_key(0);
	}
}

// the round's results: points, then sorted and ranked (career_results())
static void results(int A1){

	int 	before[RACERS], gained[RACERS], n, lead;

	// by face: each racer's own, whatever the sorting does to the order
	for(n = 0; n < RACERS; n++) before[racer(n)->face] = racer(n)->points;
	lead = career_results(A1);
	for(n = 0; n < RACERS; n++) gained[racer(n)->face] = racer(n)->points-before[racer(n)->face];
#if defined(CAREER_BENCH)
	say("CAREER round %d: you rank %d with %d points, $%d; racers hash %08lx", ++bench_round, racer(ME)->rank, racer(ME)->points, racer(ME)->money, racers_hash());
#endif
	standings(gained);
	if(lead) message("YOU LEAD ON POINTS!", "THE ADVERSARY IS WAITING");
}

// --- a race -------------------------------------------------------------------------

// race D(___1a1028h), the grid as the sign-up left it, then its results
// and the round's. 1 if it ends the game, you having beaten the Adversary
static int race(int num_cars){

	__BYTE__ 	grid[4];
	char 		buf[32];
	int 		n, place;

	career_race_track(grid);
	gba_text_screen();
	center(1, "PREPARE TO RACE");
	print(2, 4, "TRACK  %s", track_name(D(___1a1028h), buf));
	print(2, 5, "LAPS   %d", NUM_OF_LAPS);
	if((int)D(___1a1028h) < 3) print(2, 6, "PRIZE  $%d", prizes[D(___1a1028h)]);
	else gba_print(2, 6, "PRIZE  GLORY");
	for(n = 0; n < num_cars; n++){

		racer_t * 	r = racer(grid[n]);

		if((D(___1a1028h) == 3)&&(n == 0)) gba_print(4, 9+2*n, "THE ADVERSARY");
		else print(4, 9+2*n, "%2d. %s", r->rank, r->name);
		if(grid[n] == ME) gba_print(2, 9+2*n, ">");
	}
	center(18, "PRESS A TO RACE");
	wait_key(0);

	career_race_fill(num_cars, grid);
	gba_career_race(D(___1a103ch), num_cars);
	career_race_after(num_cars, grid);

	// how it went
	gba_text_screen();
	center(1, "RACE RESULTS");
	for(place = 1; place <= num_cars; place++){

		for(n = 0; (n < num_cars)&&(D(___1de7d0h+0x54*n+0x40) != (__DWORD__)place); n++);
		if(n == num_cars) continue;
		print(2, 3+2*place, "%d. %-11s %d KILLS%s", place, (char *)___1de7d0h+0x54*n, (int)D(___1de7d0h+0x54*n+0xc),
			(D(___1de7d0h+0x54*n+0x10) >= 100) ? " WRECK" : "");
		if(n == D(___1a103ch)) gba_print(0, 3+2*place, ">");
	}
	print(2, 14, "PRIZE MONEY  $%d", (int)D(___196aech));
	print(2, 15, "KILL BONUS   $%d", (int)D(___196af0h));
	center(18, "PRESS A");
	wait_key(0);

	if(career_leading()){

		if(career_leader_round()){

			message("YOU BEAT THE ADVERSARY!", "THE DEATH RALLY IS YOURS");
			return 1;
		}
		results(0);
	}
	else results(0);

	return 0;
}

// --- the sign-up ------------------------------------------------------------------------

static void signup_draw(int chosen){

	char 	buf[16];
	int 	r, n;

	for(r = 0; r < 3; r++){

		int 	col = 10*r;

		print(col, 2, (r == chosen) ? "[%s]" : " %s ", race_names[r]);
		print(col+1, 4, "%-8s", track_name(r, buf));
		print(col+1, 5, "%d LAPS", 4+r);
		print(col+1, 6, "$%d", prizes[r]);
		for(n = 0; n < 4; n++){

			const char * 	name = (n < B(___1a1f64h+r+3)) ? racer(B(___1a0ef8h+4*r+n))->name : "";

			print(col+1, 9+2*n, "%-9.9s", name);
		}
	}
}

// the round's sign-up (___3266ch()): the others sign up as you choose. 1
// if it ends the game
static int signup(void){

	int 	chosen = 0, frames = 0, r;

	career_signup_open();
	gba_text_screen();
	center(0, "SIGN UP FOR A RACE");
	gba_print(1, 18, "LEFT/RIGHT CHOOSE");
	gba_print(1, 19, "A SIGN UP, B SIT IT OUT");
	for(;;){

		u16 	k;

		signup_draw(chosen);
		k = gba_frame();
#if defined(CAREER_BENCH)
		k = KEY_A;		// the easy race, always
#endif
		if((++frames&1) == 0) ___3079ch_cdecl(0x4b);
		if(k&KEY_LEFT) chosen = (chosen+2)%3;
		if(k&KEY_RIGHT) chosen = (chosen+1)%3;
		if(k&KEY_B) break;
		if(k&(KEY_A|KEY_START)){

			r = career_signup_check(chosen);
			if(r == CAREER_NEEDS_CAR){

				message("A FASTER CAR WOULD DO BETTER", "PRESS A AGAIN TO RACE ANYWAY");
				gba_text_screen();
				center(0, "SIGN UP FOR A RACE");
				gba_print(1, 18, "LEFT/RIGHT CHOOSE");
				gba_print(1, 19, "A SIGN UP, B SIT IT OUT");
				continue;
			}
			if(r == CAREER_FULL) break;
			career_signup(chosen);
			// the rest sign up, then the grids are ordered
			while(!career_signup_full()){

				signup_draw(chosen);
				gba_frame();
				if((++frames&1) == 0) ___3079ch_cdecl(3);
			}
			signup_draw(chosen);
			career_signup_order();
			return race(4);
		}
		if(career_signup_full()) break;
	}

	// sat out, or too late: the round without you
	career_signup_rest();
	message("YOU DID NOT SIGN UP", "IN ANY RACE");
	results(1);

	return 0;
}

// the Adversary's challenge (___31008h()), once you lead. 1 if it ends the game
static int adversary(void){

	gba_text_screen();
	center(3, "THE ADVERSARY CHALLENGES YOU");
	center(6, "TR0, 9 LAPS, FOR GLORY");
	center(16, "A ACCEPT");
	center(17, "B CHICKEN OUT");
	if(wait_key(1)){

		career_adversary_decline();
		results(1);
		career_adversary_declined();
		return 0;
	}
	career_adversary_accept();

	return race(2);
}

// --- the hub ---------------------------------------------------------------------------

void career_play(void){

	int 	row = 0, n;

	gba_career_reset();
	// the racers' car colors, as ___12200h() loads them from MENU.BPA
	___1a0fb8h = (__POINTER__)copper_pal;
	___1a0fe0h = (__POINTER__)carcol_pal;
#if defined(RACE_SEED)
	srand_watcom106(RACE_SEED);
#else
	srand_watcom106(gba_cycles());
#endif
	if(!new_game()) return;
#if defined(CAREER_LEAD)
	// RACE_CFLAGS=-DCAREER_LEAD: you lead from the start, to test the
	// Adversary's race, as tools/replay --career with CAREER_LEAD=1
	racer(ME)->points = 200;
#endif

	for(;;){

		racer_t * 	me = racer(ME);
		u16 		k;

#if defined(CAREER_BENCH)
		if(bench_round >= CAREER_BENCH){

			say("CAREER BENCH done");
			for(;;) gba_frame();
		}
		if(career_leading() ? adversary() : signup()) return;
		continue;
#endif
		gba_text_screen();
		print(1, 1, "%s", me->name);
		print(1, 3, "RANK    %d", me->rank);
		print(1, 4, "POINTS  %d", me->points);
		print(1, 5, "MONEY   $%d", me->money);
		print(1, 6, "CAR     %s", car_names[me->car]);
		print(1, 7, "DAMAGE  %d%%", me->damage);
		for(;;){

			item(11, 11+2*row, career_leading() ? "FACE THE ADVERSARY" : "RACE");
			item(13, 11+2*row, "STANDINGS");
			item(15, 11+2*row, "QUIT");
			for(n = 0; n < 3; n++) gba_print(2, 11+2*n, (n == row) ? ">" : " ");
			k = gba_frame();
			if(k&KEY_UP) row = (row+2)%3;
			if(k&KEY_DOWN) row = (row+1)%3;
			if(k&(KEY_A|KEY_START)) break;
		}
		if(row == 0){

			if(career_leading() ? adversary() : signup()) return;
		}
		else if(row == 1) standings(0);
		else return;
	}
}
