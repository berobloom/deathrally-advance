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
#include "drally_shop.h"
#include "gba.h"

	extern __BYTE__ ___1a01e0h[];
	extern __BYTE__ ___1a1ef8h[];
	extern __BYTE__ ___1a1028h[];
	extern __BYTE__ ___1a103ch[];
	extern __BYTE__ ___1a1f64h[];
	extern __BYTE__ ___1a0ef8h[];
	extern __BYTE__ ___1a1ff1h[];
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
	extern const cardata_t ___18e298h[6];
	extern __BYTE__ ___1a1ee4h[];
	extern __BYTE__ ___185a14h_UseWeapons[];

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

// --- saving --------------------------------------------------------------------------

// the cartridge's battery-backed SRAM, read and written a byte at a time:
// the career at CAREER_SAVE, as the desktop's DR.SG files hold it, and
// the lap records at RECORDS_SAVE, as its DR.CFG does
#define SAVE_RAM		((volatile u8 *)0x0e000000)
#define CAREER_SAVE		0x0000
#define CAREER_MAGIC		0x31535244	// "DRS1"
#define RECORDS_SAVE		0x1000
#define RECORDS_MAGIC		0x31525244	// "DRR1"

	extern record_t ___19f750h[6][18];

// for the emulators and flash carts that look for it to know the save type
__attribute__((used)) static const char sram_id[] = "SRAM_V113";

static u32 sram_sum(const u8 * p, int n){

	u32 	h = 0x811c9dc5;

	while(n--) h = (h^*p++)*0x01000193;

	return h;
}

static void sram_put(int at, const void * src, int n){

	const u8 	* p = src;

	while(n--) SAVE_RAM[at++] = *p++;
}

static void sram_get(int at, void * dst, int n){

	u8 	* p = dst;

	while(n--) *p++ = SAVE_RAM[at++];
}

// a block: its magic, its checksum, then it
static void block_save(int at, u32 magic, const void * src, int n){

	u32 	head[2] = { magic, sram_sum(src, n) };

	// the magic last, so that a block cut short by power off isn't taken
	sram_put(at+8, src, n);
	sram_put(at+4, &head[1], 4);
	sram_put(at, &head[0], 4);
}

static int block_load(int at, u32 magic, void * dst, int n){

	u32 	head[2];

	(void)*(volatile const char *)sram_id;	// kept in the ROM
	sram_get(at, head, 8);
	if(head[0] != magic) return 0;
	sram_get(at+8, dst, n);

	return sram_sum(dst, n) == head[1];
}

// the career as it stands, and the lap records with it
static void career_store(void){

	EWRAM_BSS static saved_game_t 	sg;

	career_save(&sg, racer(ME)->name);
	block_save(CAREER_SAVE, CAREER_MAGIC, &sg, sizeof(sg));
	block_save(RECORDS_SAVE, RECORDS_MAGIC, ___19f750h, sizeof(record_t)*6*18);
}

// the game over: no career to go on with
static void career_forget(void){

	u32 	none = 0;

	sram_put(CAREER_SAVE, &none, 4);
	block_save(RECORDS_SAVE, RECORDS_MAGIC, ___19f750h, sizeof(record_t)*6*18);
}

// the saved career, if there's one, and the lap records. 1 if there was
static int career_restore(saved_game_t * sg){

	EWRAM_BSS static record_t 	rec[6][18];

	if(block_load(RECORDS_SAVE, RECORDS_MAGIC, rec, sizeof(rec))) memcpy(___19f750h, rec, sizeof(rec));

	return block_load(CAREER_SAVE, CAREER_MAGIC, sg, sizeof(*sg));
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

static void speech(const char40 * t, int n_lines);
static void text_lines(int row0, int rows, const char * const * t, int n_lines);

// after the grids are ordered (___32230h()): the others' cars repaired,
// and the sabotage you paid for done
static int sabotage(void){

	char40 		t[7] = { "", "", "", "", "", "", "" };
	int 		victim;

	victim = career_signup_sabotage(gba_cycles());
	if(victim < 0) return 0;
	snprintf(t[0], 40, "[Sabotage");
	snprintf(t[2], 40, "Money talks, and the damage is");
	snprintf(t[3], 40, "done, %d%% worth! Sabotage says that", (int)racer(victim)->damage);
	snprintf(t[4], 40, "%s is going down and staying", racer(victim)->name);
	snprintf(t[5], 40, "down. That's dough baked luck for");
	snprintf(t[6], 40, "you. And luck, she ain't no lady.");
	speech(t, 7);

	return 1;
}

// ___31868h(): maybe a special offer after the sign-up, the hitman's or the
// steroids', yes or no
static void offer(void){

	char 		l3[48], l5[48], l6[48];
	const char 	* t[8];
	int 		ev, pay, target;

	ev = career_offer(&pay, &target);
	if(ev == CAREER_EVENT_NONE) return;
	if(ev == CAREER_EVENT_HITMAN){

		snprintf(l3, sizeof(l3), "that [%s won't reach the", racer(target)->name);
		snprintf(l5, sizeof(l5), "[it'll pay you $%d.", pay);
		t[0] = "[Wanna be my hitman, driver?"; t[1] = "";
		t[2] = "Wanna get rich, tough guy? OK."; t[3] = "[Here's the deal{. I have put some";
		t[4] = "major funds on this race, betting"; t[5] = l3;
		t[6] = "[finish line{. Make me right, and"; t[7] = l5;
	}
	else {

		snprintf(l6, sizeof(l6), "[Extra $%d{, anyone? You? This is", pay);
		t[0] = "[A slick steroid run, anyone?"; t[1] = "";
		t[2] = l6; t[3] = "easy as shifting gears 1-2-3.";
		t[4] = "[Just pick up some pills along the"; t[5] = "[way, and get them to the finish";
		t[6] = "[line before things get crowded{, be"; t[7] = "first, like, floor it, like.";
	}
	gba_text_screen();
	center(0, "SPECIAL OFFER");
	text_lines(3, 13, t, 8);
	center(17, "A DEAL");
	center(18, "B NO DEAL");
	if(!wait_key(1)) career_offer_accept(ev, target);
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
			if(!sabotage()) offer();
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

// --- the shop ------------------------------------------------------------------------

// the shop as shop_main() has it, in rows: the car on show, the engine,
// tires and armor, the repairs, and out. career.c does the sums
#define SHOP_ROW0		5	// the items' first row
#define SHOP_TEXT		12	// the game's words about the item, rows 12-17
#define SHOP_TEXT_ROWS		6
#define SHOP_BACK		5
#define SHOP_CAR_X		184	// the car on show, turning top right
#define SHOP_CAR_Y		0

static const char * const shop_items[6] = { "CAR", "ENGINE", "TIRES", "ARMOR", "REPAIR", "DONE" };

// the game's n lines in rows row0 to row0+rows-1, without its colour marks
// ([ and {): the first line by itself, the rest run on, word-wrapped
static void text_lines(int row0, int rows, const char * const * t, int n_lines){

	char 	para[12*48+16], * w;
	int 	row = row0, n, len, i, first;

	for(n = 0; n < rows; n++) gba_print(1, row0+n, "                            ");
	para[0] = 0;
	// the heading is the first line with words, the screens' empty ones
	// before it left out
	for(first = 0; (first < n_lines)&&!t[first][0]; first++);
	for(n = first; n < n_lines; n++){

		len = strlen(para);
		for(i = 0; t[n][i]&&(len < (int)sizeof(para)-2); i++){

			// the loan shark's speeches are indented: not the spaces
			if((len > 0)&&(para[len-1] == ' ')&&(t[n][i] == ' ')) continue;
			if((t[n][i] != '[')&&(t[n][i] != '{')) para[len++] = t[n][i];
		}
		// a line by itself after the first, the rest run together, a
		// word broken with a hyphen joined up again
		if(n == first) para[len++] = '\n';
		else if((len > 0)&&(para[len-1] != '-')&&(para[len-1] != ' ')&&(para[len-1] != '\n')) para[len++] = ' ';
		para[len] = 0;
	}
	for(w = para; *w&&(row < row0+rows); row++){

		char 	line[29];

		while(*w == ' ') w++;
		// as much as fits, broken at a space or a line's end
		for(len = 0; w[len]&&(w[len] != '\n')&&(len < 28); len++);
		if(w[len]&&(w[len] != '\n')&&(w[len] != ' ')){

			for(i = len; (i > 0)&&(w[i] != ' '); i--);
			if(i > 0) len = i;
		}
		memcpy(line, w, len);
		line[len] = 0;
		gba_print(1, row, line);
		w += len;
		if(*w == '\n') w++;
	}
}

// the same for lines of 40
static void text_at(int row0, int rows, const char40 * t, int n_lines){

	char 		buf[12][41];	// a line of 40 may have no 0
	const char 	* p[12];
	int 		n;

	for(n = 0; (n < n_lines)&&(n < 12); n++){

		memcpy(buf[n], t[n], 40);
		buf[n][40] = 0;
		p[n] = buf[n];
	}
	text_lines(row0, rows, p, n);
}

// the game's 6 lines of 40 under the shop's or the market's items
static void shop_text(const char40 * t){

	text_at(SHOP_TEXT, SHOP_TEXT_ROWS, t, 6);
}

// a screen of the game's words, until A
static void speech(const char40 * t, int n_lines){

	gba_text_screen();
	text_at(3, 13, t, n_lines);
	center(18, "PRESS A");
	wait_key(0);
}

// the same, of lines of any length
static void speech_lines(const char * const * t, int n_lines){

	gba_text_screen();
	text_lines(3, 13, t, n_lines);
	center(18, "PRESS A");
	wait_key(0);
}

// what to say of the item when it's chosen, as the shop's own panels do
static void shop_about(int item){

	racer_t * 		me = racer(ME);
	const cardata_t * 	c = &___18e298h[me->car];
	int 			n = career_shop_level(item), most = career_shop_most(item);
	static const char40 	done[6] = { "[DONE", "", "Out of the shop, back to the", "garage.", "", "" };
	int 			d = me->damage, k;

	switch(item){
	case CAREER_SHOP_CAR:
		shop_text(D(___185a14h_UseWeapons) ? ___18e298h[D(___1a1ee4h)].txt_winfo : ___18e298h[D(___1a1ee4h)].txt_info);
		break;
	case CAREER_SHOP_ENGINE: shop_text((n < most) ? c->txt_engine_upgrades[n] : MaxUpgrades[0]); break;
	case CAREER_SHOP_TIRES: shop_text((n < most) ? ___1914c0h[n] : MaxUpgrades[1]); break;
	case CAREER_SHOP_ARMOR: shop_text((n < most) ? ___191c40h[n] : MaxUpgrades[2]); break;
	case CAREER_SHOP_REPAIR:
		// ___269e4h()'s: 0 for a wreck, up to 11 for none
		k = (d == 0) ? 11 : (d >= 100) ? 0 : 10-d/10;
		shop_text(___194170h[k]);
		break;
	default: shop_text(done); break;
	}
}

// ___28ab4h_cdecl()'s words when you're short: 0 if you aren't
static int shop_short(int price){

	char40 		t[6] = { "", "", "", "", "", "" };
	racer_t * 	me = racer(ME);

	if(price <= me->money) return 0;
	snprintf(t[2], 40, "No dough, no go, bro. No money,");
	snprintf(t[3], 40, "honey. You are $%d short.", price-me->money);
	snprintf(t[4], 40, "Gotta earn it. Go do it already.");
	shop_text(t);

	return 1;
}

static void shop_draw(int row){

	racer_t * 	me = racer(ME);
	char 		detail[16], price[12];
	int 		item, p, n;

	print(1, 1, "MONEY   $%-8d", me->money);
	print(1, 2, "DAMAGE  %d%%  ", me->damage);
	print(1, 3, "TRADE-IN $%-7d", career_shop_trade_in());
	for(item = 0; item < 6; item++){

		p = (item < SHOP_BACK) ? career_shop_price(item) : CAREER_SHOP_NONE;
		n = career_shop_level(item);
		detail[0] = price[0] = 0;
		if(item == CAREER_SHOP_CAR) snprintf(detail, sizeof(detail), "<%s>", car_names[D(___1a1ee4h)]);
		else if(item == CAREER_SHOP_REPAIR) snprintf(detail, sizeof(detail), (p == CAREER_SHOP_NONE) ? "NONE" : "10%% OFF");
		else if(item < SHOP_BACK) snprintf(detail, sizeof(detail), (p == CAREER_SHOP_NONE) ? "ALL %d" : "%d OF %d", n, career_shop_most(item));
		if(p != CAREER_SHOP_NONE) snprintf(price, sizeof(price), (p < 0) ? "+$%d" : "$%d", (p < 0) ? -p : p);
		print(0, SHOP_ROW0+item, "%c %-6s %-13s%8s", (item == row) ? '>' : ' ', shop_items[item], detail, price);
	}
	gba_print(1, 19, (row == CAREER_SHOP_CAR) ? "LEFT/RIGHT CAR  A BUY  B OUT" : "A BUY  B OUT                ");
}

// the car on show turning: the one you'd buy on its row, else yours
static void shop_car(int row){

	gba_show_car((row == CAREER_SHOP_CAR) ? (int)D(___1a1ee4h) : (int)racer(ME)->car, SHOP_CAR_X, SHOP_CAR_Y);
}

// shop___28e40h()'s yes or no for a new car: 1 for yes
static int shop_confirm_car(void){

	char40 		t[6] = { "", "", "", "", "", "" };
	const cardata_t * 	c = &___18e298h[D(___1a1ee4h)];
	int 		refund = career_shop_trade_in(), cost = career_shop_price(CAREER_SHOP_CAR);

	snprintf(t[0], 40, "You would get a $%d refund", refund);
	snprintf(t[2], 40, "from your old car and upgrades.");
	if(cost >= 0) snprintf(t[3], 40, "%s would cost $%d.", c->name, cost);
	else snprintf(t[3], 40, "Money returned: $%d.", -cost);
	snprintf(t[4], 40, (cost >= 0) ? "Purchase it?" : "Buy a %s?", c->name);
	shop_text(t);
	gba_print(1, 19, "A YES  B NO                 ");
	for(;;){

		u16 	k = gba_frame();

		gba_turn_car();
		if(k&(KEY_A|KEY_START)) return 1;
		if(k&KEY_B) return 0;
	}
}

// the car on show in your color, one of COPPER.PAL's (___1a0fb8h),
// as the shop shows it (___11378h_cdecl())
static void my_color(void){

	const __BYTE__ 	* c = (const __BYTE__ *)___1a0fb8h+3*racer(ME)->color;

	gba_car_color(c[0], c[1], c[2]);
}

// a new car's color, as the shop asks for it once it's bought: left and
// right two along the copper colors, A done
static void shop_color(void){

	racer_t * 	me = racer(ME);
	char 		buf[32];
	int 		n;

	for(;;){

		u16 	k;

		my_color();
		snprintf(buf, sizeof(buf), "COLOR  <%4d >", (int)me->color/2+1);
		gba_print(1, 11, buf);
		gba_print(1, 19, "LEFT/RIGHT COLOR, A DONE    ");
		k = gba_frame();
		gba_turn_car();
		// L and R 16 along
		n = (k&KEY_LEFT) ? -2 : (k&KEY_RIGHT) ? 2 : (k&KEY_L) ? -32 : (k&KEY_R) ? 32 : 0;
		n += (int)me->color;
		me->color = (n < 0) ? 0 : (n > 0xfe) ? 0xfe : n;
		if(k&(KEY_A|KEY_START)) break;
	}
	gba_print(1, 11, "              ");
	gba_print(1, 19, "                            ");
}

// the shop (shop_main()), until DONE or B. Buying shows what was bought
// until another row is chosen
static void shop(void){

	racer_t * 	me = racer(ME);
	int 		row = CAREER_SHOP_CAR, n, p;

	// the car after yours on show, as ___24548h() starts it
	D(___1a1ee4h) = ((int)me->car >= 5) ? 5 : me->car+1;
	gba_text_screen();
	my_color();
	center(0, "THE SHOP");
	shop_car(row);
	shop_about(row);
	for(;;){

		u16 	k;

		shop_draw(row);
		k = gba_frame();
		gba_turn_car();
		if(k&KEY_B) break;
		if(k&(KEY_UP|KEY_DOWN)){

			row = (row+((k&KEY_UP) ? 5 : 1))%6;
			shop_car(row);
			shop_about(row);
			continue;
		}
		if((row == CAREER_SHOP_CAR)&&(k&(KEY_LEFT|KEY_RIGHT))){

			D(___1a1ee4h) = (D(___1a1ee4h)+((k&KEY_LEFT) ? 5 : 1))%6;
			shop_car(row);
			shop_about(row);
			continue;
		}
		if(!(k&(KEY_A|KEY_START))) continue;
		if(row == SHOP_BACK) break;

		p = career_shop_price(row);
		if(p == CAREER_SHOP_NONE){

			shop_about(row);
			continue;
		}
		if(shop_short(p)) continue;
		switch(row){
		case CAREER_SHOP_CAR:
			if(!shop_confirm_car()){

				shop_about(row);
				break;
			}
			career_shop_buy(row);
			gba_show_car(me->car, SHOP_CAR_X, SHOP_CAR_Y);
			shop_color();
			// shown as ___27078h() does it, the new car's own words,
			// and turning until another row or car is chosen
			shop_text(___18e298h[me->car].txt_bought);
			break;
		case CAREER_SHOP_REPAIR:
			career_shop_buy(row);
			shop_about(row);
			break;
		default:
			// the installed words go by the level it was
			n = career_shop_level(row);
			career_shop_buy(row);
			shop_text((row == CAREER_SHOP_ENGINE) ? ___191100h[n] : (row == CAREER_SHOP_TIRES) ? ___191880h[n] : ___192000h[n]);
			break;
		}
	}
	gba_show_car(-1, 0, 0);
}

// --- the underground market --------------------------------------------------------

// underground_main(): the loan shark, then what's on sale until the next
// race. career.c does the sums
#define MARKET_DONE		5

static const char * const market_items[6] = { "LOAN SHARK", "MINES", "SPIKED BUMPERS", "ROCKET FUEL", "SABOTAGE", "DONE" };

// the market's own words, which its desktop functions print as they draw
static const char40 market_out[6] = { "[OUT OF STOCK", "", "Sorry, no can do. That's out of", "stock. Hold your horse(power)s", "till after the next race.", "" };
static const char40 market_no_sabotage[6] = { "[No sabotage. No way.", "", "What? You crazy, man? Nobody", "touches the Adversary's auto-", "mobile and lives, nobody.", "" };
static const char40 market_denied[6] = { "[Loan denied, Vagabond driver.", "", "This is a business, not a charity.", "We don't invest in unproven wild", "cards. So buzz off, bug.", "" };
static const char40 market_paid[6] = { "[Loan paid back.", "", "It's all here. I knew right from", "the start that I could trust you.", "Let's do this again anytime.", "" };
static const char40 market_done[6] = { "[DONE", "", "Out of the Underground Market,", "back to the garage.", "", "" };

static void market_about(int item){

	racer_t * 	me = racer(ME);
	char40 		t[6] = { "", "", "", "", "", "" };

	switch(item){
	case CAREER_MARKET_LOAN:
		// ___2d294h(): his offer, the smallest to a Vagabond's driver,
		// or what you owe
		if(career_loan_owed() == 0){

			shop_text(___1923c0h[(me->car == 0) ? 4 : 5-me->car]);
			break;
		}
		snprintf(t[0], 40, "Pay back $%d.", career_loan_owed());
		snprintf(t[2], 40, "Petrol brain, you owe me. You'd");
		snprintf(t[3], 40, "better be here to pay me back, or");
		snprintf(t[4], 40, "else we have nothing to discuss.");
		shop_text(t);
		break;
	case MARKET_DONE:
		shop_text(market_done);
		break;
	default:
		if(career_market_state(item) == 1) shop_text(___193720h[item-1]);
		else if((item == CAREER_MARKET_SABOTAGE)&&career_leading()) shop_text(market_no_sabotage);
		else shop_text(market_out);
		break;
	}
}

static void market_draw(int row){

	racer_t * 	me = racer(ME);
	char 		price[12];
	int 		item, owed = career_loan_owed();

	print(1, 1, "MONEY   $%-8d", me->money);
	if(owed) print(1, 2, "YOU OWE $%-6d %d RACE%s LEFT ", owed, 4-(int)me->loanshark_counter, (me->loanshark_counter == 3) ? "" : "S");
	else gba_print(1, 2, "                             ");
	for(item = 0; item < 6; item++){

		price[0] = 0;
		if(item == CAREER_MARKET_LOAN){

			if(owed) snprintf(price, sizeof(price), "-$%d", owed);
			else if(career_loan_amount()) snprintf(price, sizeof(price), "+$%d", career_loan_amount());
		}
		else if(item < MARKET_DONE){

			if(career_market_state(item) == 1) snprintf(price, sizeof(price), "$%d", career_market_price(item));
			else snprintf(price, sizeof(price), "SOLD");
		}
		print(0, SHOP_ROW0+item, "%c %-19s%8s", (item == row) ? '>' : ' ', market_items[item], price);
	}
	gba_print(1, 19, "A BUY  B OUT                ");
}

// A on a row: the loan taken or paid back, or the item bought
static void market_buy(int row){

	racer_t * 	me = racer(ME);
	char40 		granted[6];

	if(row == CAREER_MARKET_LOAN){

		if(career_loan_owed()){

			if(shop_short(career_loan_owed())) return;
			career_loan_pay();
			shop_text(market_paid);
		}
		else if(!career_loan_amount()) shop_text(market_denied);
		else {

			career_loan_take();
			// ___26fach()'s, without its line about the Esc key
			memcpy(granted, ___192870h[me->loanshark_type], sizeof(granted));
			granted[5][0] = 0;
			shop_text(granted);
		}
		return;
	}
	if(career_market_state(row) != 1){

		market_about(row);
		return;
	}
	if(shop_short(career_market_price(row))) return;
	career_market_buy(row);
	shop_text(___193db0h[row-1]);
}

static void market(void){

	// ___1716ch(), the first time
	static const char * const 	welcome[10] = { "", "[Psst...Yeah you, driver.", "",
		"Jealousy burns you like a dragon inside, you", "[want that road all to yourself?{ So jinx",
		"those rival suckers, let them go out in a", "blaze; you take the glory. [Welcome to the",
		"[Underground Market.{ What you want, we got.", "Dig out the dough, this gonna cost you.", "" };
	int 	row = CAREER_MARKET_LOAN;

	career_market_open();
	if(career_market_welcome()) speech_lines(welcome, 10);
	gba_text_screen();
	center(0, "THE UNDERGROUND MARKET");
	market_about(row);
	for(;;){

		u16 	k;

		market_draw(row);
		k = gba_frame();
		if(k&KEY_B) break;
		if(k&(KEY_UP|KEY_DOWN)){

			row = (row+((k&KEY_UP) ? 5 : 1))%6;
			market_about(row);
			continue;
		}
		if(!(k&(KEY_A|KEY_START))) continue;
		if(row == MARKET_DONE) break;
		market_buy(row);
	}
}

// ___168b4h(): after three races, the loan shark comes for his money
static void loan_collect(void){

	static const char40 	paid[8] = { "[It's pay-back time, driver.", "",
		"This is what I like to see, a guy", "who keeps his part of the bargain.",
		"Ah, the sweet smell of money. Come", "to papa, my babies. Real nice doing",
		"business with you, thunderboy.", "Thanks, and good racing riddance." };
	static const char40 	hurt[8] = { "[It's pay-back time, driver.", "",
		"Tut tut, hotshot, I can't see my", "money anywhere. I ain't amused,",
		"joker. A deal is a deal. I got a", "reputation to look after. Too bad.",
		"Nothing personal, but I got to hurt", "you some now. Hurt 'im, boyz." };

	speech(career_loan_collect(1) ? paid : hurt, 8);
}

// the shop's welcome after your first race (___164d0h()), and the game's
// before it (___17248h())
static const char * const 	shop_welcome[10] = { "", "[Welcome to The Shop", "",
	"Zillions more miles to go. That junk of yours", "is no juggernaut, she won't take you all",
	"the way. [You need to gear up{, to upgrade,", "make headlong headway, strive to stride,",
	"stride to strive, evolve, improve, better,", "best. [Buy & be one shining perfection.{", "" };
static const char * const 	game_welcome[10] = { "", "[Welcome to Death Rally.", "",
	"The race is on, in [Death Rally{, where life is", "short & hot, fast & deadly. Watch the road,",
	"hear the rumble. [Start out easy{, get the", "feel of it. But remember: by the rules or",
	"fry the rules, [play to win{. 'nuff said. Now", "[ride your machine{, your motor-war-chariot.", "" };

// a winning streak, the reaper's or not a scratch (___14dc4h() and its
// kin): the game's lines, the one at spcf_i your car's
static void event_paid(const event_text_t * e){

	const char 	* t[10];
	int 		n;

	for(n = 0; n < 10; n++) t[n] = (n == e->spcf_i) ? e->spcf[racer(ME)->car] : e->common[n];
	speech_lines(t, 10);
}

// the steroids' or the hit's outcome (___15808h(), ___15e3ch()): what it
// paid, or the fine
static void event_deal(int ev){

	static const char * const 	no_pills[8] = { "[Where're my steroids?", "",
		"[The whole deal went sour{ because", "of your incompetence. A few slick",
		"turns of the wheel, but no, nope!", "I'm not happy, and when I'm not",
		"happy I am mad, and when I am", "mad [bad things happen - to you!" };
	static const char * const 	missed[8] = { "[Your hit missed!", "",
		"You sissy Sunday driver. [The", "[simplest job, and you screw it up!",
		"I bet you are afraid of the dark as", "well. I'll show you what happens",
		"when you break your promises to me.", "[I'm gonna make you pay!" };
	char 		a[64], b[48];
	const char 	* t[8];
	int 		v = career_event_apply(ev);

	if(v <= 0){

		speech_lines((ev == CAREER_EVENT_STEROIDS) ? no_pills : missed, 8);
		return;
	}
	if(ev == CAREER_EVENT_STEROIDS){

		snprintf(b, sizeof(b), "[here is prime stuff! $%d, to you.", v);
		t[0] = "[What motorized steroid peddling!"; t[1] = "";
		t[2] = "Wow, my main man. I didn't know"; t[3] = "there was that much smoke in the";
		t[4] = "whole burning hell, and through"; t[5] = "it all, bearing gifts like some";
		t[6] = "blessed beast, you came. And [this"; t[7] = b;
	}
	else {

		snprintf(a, sizeof(a), "beautiful, [%s is in a car-", (const char *)___1a1ff1h);
		snprintf(b, sizeof(b), "[coffin grave. $%d, your money.", v);
		t[0] = "[You totally slew that car."; t[1] = "";
		t[2] = "A killer race! The bullets took the"; t[3] = "sky like clouds of locusts, and";
		t[4] = "the screams of motors were a"; t[5] = "banshee wail. [Death!{ You were";
		t[6] = a; t[7] = b;
	}
	speech_lines(t, 8);
}

// what shop_main() tells you when you come back from a race, each in turn
static void events(void){

	static const char * const 	lapped[10] = { "", "", "[You were lapped, you loser!", "",
		"The pain, the humiliation! Your stunt", "made me the laughing stock of the",
		"races. What were you trying to do,", "mimic slow-motion? No one is going",
		"to take me seriously after this.", "[No money or points awarded." };
	static const char * const 	too_slow[10] = { "", "", "[Too slow, no dough!", "",
		"Death Rally is for winners. You", "totally lost this race. [All the",
		"[bonus $$ you collected on the way", "[are null and void.{ You get nothing,",
		"get it? Zero, none, naught, nix,", "nada, zilch. Next time, be better." };
	int 	ev;

	while((ev = career_event_next()) != CAREER_EVENT_NONE){

		switch(ev){
		case CAREER_EVENT_WELCOME: speech_lines(shop_welcome, 10); break;
		case CAREER_EVENT_LAPPED: speech_lines(lapped, 10); break;
		case CAREER_EVENT_STREAK: career_event_apply(ev); event_paid(&event_streak); break;
		case CAREER_EVENT_REAPER: career_event_apply(ev); event_paid(&event_reaper); break;
		case CAREER_EVENT_INCONCEIVABLE: career_event_apply(ev); event_paid(&event_inconceivable); break;
		case CAREER_EVENT_STEROIDS:
		case CAREER_EVENT_HITMAN: event_deal(ev); break;
		case CAREER_EVENT_LOAN: loan_collect(); break;
		case CAREER_EVENT_TOO_SLOW: speech_lines(too_slow, 10); break;
		}
		career_event_clear(ev);
	}
}

// a saved career to go on with, or a new one: 1 for the saved one, which
// it loads, 0 for new, -1 for B
static int continue_or_new(void){

	EWRAM_BSS static saved_game_t 	sg;
	const racer_t * 	me;
	int 			row = 0;

#if defined(CAREER_BENCH)
	return 0;
#endif
	if(!career_restore(&sg)) return 0;
	me = &sg.racers[sg.me];
	gba_text_screen();
	center(1, "CAREER");
	print(6, 6, "%s", me->name);
	print(6, 7, "RANK %d, %d POINTS", (int)me->rank, (int)me->points);
	print(6, 8, "$%d, %s", (int)me->money, car_names[me->car]);
	gba_print(1, 18, "A CHOOSE, B BACK");
	for(;;){

		u16 	k;

		item(4, row ? 13 : 4, "CONTINUE");
		item(13, row ? 13 : 4, "NEW CAREER");
		k = gba_frame();
		if(k&(KEY_UP|KEY_DOWN)) row = !row;
		if(k&KEY_B) return -1;
		if(k&(KEY_A|KEY_START)) break;
	}
	if(row) return 0;
	career_load(&sg);

	return 1;
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
	n = continue_or_new();
	if(n < 0) return;
	if(n == 0){

		if(!new_game()) return;
#if defined(CAREER_LEAD)
		// RACE_CFLAGS=-DCAREER_LEAD: you lead from the start, to test
		// the Adversary's race, as tools/replay --career with
		// CAREER_LEAD=1
		racer(ME)->points = 200;
#endif
		// a new game goes straight to the sign-up, welcomed
		// (shop_main(), ___3266ch()), the shop's welcome after
		speech_lines(game_welcome, 10);
		if(career_leading() ? adversary() : signup()){

			career_forget();
			return;
		}
	}

	for(;;){

		racer_t * 	me = racer(ME);
		u16 		k;

		events();
#if defined(CAREER_BENCH)
		if(bench_round >= CAREER_BENCH){

			say("CAREER BENCH done");
			for(;;) gba_frame();
		}
		career_shop_continue();
		if(career_leading() ? adversary() : signup()) return;
		continue;
#endif
		career_store();
		gba_text_screen();
		print(1, 1, "%s", me->name);
		print(1, 3, "RANK    %d", me->rank);
		print(1, 4, "POINTS  %d", me->points);
		print(1, 5, "MONEY   $%d", me->money);
		print(1, 6, "CAR     %s", car_names[me->car]);
		print(1, 7, "DAMAGE  %d%%", me->damage);
		if(career_loan_owed()) print(1, 8, "LOAN    $%d, %d RACE%s LEFT", career_loan_owed(), 4-(int)me->loanshark_counter, (me->loanshark_counter == 3) ? "" : "S");
		// after events(), shop_main()'s end of the road: a wreck you
		// can't pay to repair
		if(career_shop_broke()){

			message("END OF THE ROAD, DRIVER", "A WRECK AND NO MONEY TO FIX IT");
			career_forget();
			return;
		}
		for(;;){

			item(10, 10+2*row, career_leading() ? "FACE THE ADVERSARY" : "RACE");
			item(12, 10+2*row, "SHOP");
			item(14, 10+2*row, "UNDERGROUND MARKET");
			item(16, 10+2*row, "STANDINGS");
			item(18, 10+2*row, "QUIT");
			for(n = 0; n < 5; n++) gba_print(2, 10+2*n, (n == row) ? ">" : " ");
			k = gba_frame();
			if(k&KEY_UP) row = (row+4)%5;
			if(k&KEY_DOWN) row = (row+1)%5;
			if(k&(KEY_A|KEY_START)) break;
		}
		if(row == 0){

			// CONTINUE's check
			if(!career_shop_can_race()) message("REPAIR YOUR CAR FIRST", "R-E-P-A-I-R");
			else {

				career_shop_continue();
				if(career_leading() ? adversary() : signup()){

					career_forget();
					return;
				}
			}
		}
		else if(row == 1) shop();
		else if(row == 2) market();
		else if(row == 3) standings(0);
		else return;
	}
}
