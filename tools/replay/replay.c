// replay: runs a recorded race (make ticklog) through this build's
// race_logic() on the desktop, without the rest of the game.
//
//   replay TICKLOG.BIN [OUT.BIN]
//
// Says in which frames the hash differs from the recording's. With OUT.BIN
// it writes the recording again with this build's hashes, which is how a
// change to the race code gets a recording to check gba/replay against,
// without racing again: the input and the rest of each frame's changes
// stay as they were recorded.
//
//   replay --setup TICKPRE.BIN BAKEDIR [TICKLOG.BIN]
//
// Sets a race up the way the GBA does, with race_setup() and the data
// tools/gbabake baked into BAKEDIR, from the state the menus left before
// a recorded race (TICKPRE.BIN, see race_ticklog_pre()). Prints the hash
// of the globals after it and, given the race's TICKLOG.BIN, which of
// them differ from how the race started there.
//
//   replay --quick TRACK BAKEDIR SEED
//
// The same for a quick race (race_quick()) with gba/race's cars, to
// compare with the hash gba/race logs. VAR_HASHES=1 also prints a hash of
// each global, as gba/race does built with RACE_CFLAGS=-DVAR_HASHES.
//
//   replay --race TRACK BAKEDIR SEED
//
// And runs it to its end as gba/race's benchmark does (RACE_CFLAGS=
// "-DRACE_BENCH -DRACE_SEED=n"): the AI drives every car, a tick at a time.
// Prints the hash of the globals and of the track image at the end, which
// gba/race logs too.
//
//   replay --career BAKES SEED ROUNDS
//
// Plays ROUNDS rounds of a career as gba/race's career benchmark does
// (RACE_CFLAGS="-DRACE_BENCH -DCAREER_BENCH=ROUNDS -DRACE_SEED=SEED"):
// always the easy race, or the Adversary once you lead, the AI driving
// your car. CAREER_LEAD=1 gives you the lead from the start. Each race's
// track comes from BAKES/bake-trN, as gba/race's
// build directory has them. Prints the hash of the globals after each
// race and of the racers after each round, which gba/race logs too.
//
// TRACK with an r after it (3r) is the track reversed. BAKEDIR files:GAMEDIR
// sets the race up from the game's files in GAMEDIR instead, as the
// desktop game does (not in replay-tiles), to check the baked data against.
// WEAPONS=0 races without guns or mines.

#include <unistd.h>
#include "drally.h"
#include "drally_structs_free.h"
#include "race_replay.h"
#include "drally_race.h"
#include "drally_structs_fixed.h"
#include "drally_career.h"

	extern __BYTE__ ___1e6ed0h[];
	extern __BYTE__ ___1de580h[];
	extern __BYTE__ ___1de7d0h[];

void race_logic(void);
void race___40164h(void);

	extern __BYTE__ ___243ce0h[];
	extern __BYTE__ ___2438c8h[];
	extern int CURRENT_VIEWPORT_W;
	extern int CURRENT_VIEWPORT_X;
	extern int CURRENT_VIEWPORT_CENTER_X;
	extern int TRX_WIDTH;
	extern int TRX_HEIGHT;

// what the game's keyboard and timer give race_render() and the timer
// interrupt, as in gba/race: no keys, the AI drives, and the 70 Hz
// counter the animations go by counts the timer's ticks
__BYTE__ kmap[256];
static __DWORD__ timer_ticks;
__DWORD__ __GET_FRAME_COUNTER(void){ return timer_ticks; }

// sound isn't replayed
void dRally_Sound_pushEffect(__BYTE__ channel, __BYTE__ n, __DWORD__ unk, __DWORD__ a0, __DWORD__ a1, __DWORD__ a2){}
void dRally_Sound_freeEffectChannel(__BYTE__ ch_num){}
void dRally_Sound_adjustEffect(__BYTE__ channel, __DWORD__ vol, __DWORD__ freq, __DWORD__ balance){}

// what the game's files loader calls when it runs out of memory, before
// it says so and exits
void dRally_System_clean(void){}
void __VGA3_SETMODE(void){}

static void * read_file(const char * path, long * size){

	FILE * 	fd;
	void * 	p;

	if(!(fd = fopen(path, "rb"))){ perror(path); exit(1); }
	fseek(fd, 0, SEEK_END);
	*size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	p = malloc(*size);
	if(fread(p, 1, *size, fd) != (size_t)*size){ perror(path); exit(1); }
	fclose(fd);

	return p;
}

static int setup(int argc, char * argv[]){

	long 		size;
	__DWORD__ * 	pre = read_file(argv[2], &size);
	__BYTE__ * 	flat = malloc(race_replay_size()), * start;
	unsigned 	i, o, k, diffs = 0;

	if(pre[2] != race_replay_size()){ fprintf(stderr, "%s: from another build\n", argv[2]); return 1; }
	race_replay_scatter((__BYTE__ *)(pre+3));
	race_baked = race_baked_read(argv[3]);
	race_setup(pre[0], pre[1], &race_loaders_baked);
	race_replay_gather(flat);
	printf("set up: globals hash %08x\n", race_replay_hash(flat));
	if(argc < 5) return 0;

	// the recording's globals at its first frame
	{
		__DWORD__ * 	log = read_file(argv[4], &size), * p = log+4+log[3];

		for(i = 0; i < RACE_REPLAY_NBUFS; i++) p += 1+(p[0]+3)/4;
		start = (__BYTE__ *)p;
	}
	for(i = o = 0; i < race_replay_count(); o += race_replay_var_size(i++)){

		for(k = 0; k < race_replay_var_size(i); k++){

			if(flat[o+k] == start[o+k]) continue;
			printf("%s+0x%x: %02x, recorded %02x\n", race_replay_var_name(i), k, flat[o+k], start[o+k]);
			diffs++;
		}
	}
	printf("%u bytes differ from the recording's start\n", diffs);

	return 0;
}

// race_quick() on TRACK, reversed if it ends in r, and the loaders BAKEDIR names
static const race_loaders_t * quick_setup(const char * track, const char * bakes, __DWORD__ seed){

	static const int 	models[4] = { 3, 5, 4, 2 };
	static const __BYTE__ 	colors[4][3] = { {63, 8, 4}, {8, 24, 63}, {63, 52, 4}, {8, 52, 12} };

	race_quick(atoi(track), models, colors, seed);
	if(track[strlen(track)-1] == 'r') D(___1de7d0h+0x48) = 1;
	// WEAPONS=0: a race without guns or mines, as gba/race can choose
	if(getenv("WEAPONS")&&!atoi(getenv("WEAPONS"))){

		int 	n;

		for(n = 0; n < 4; n++) D(___1de7d0h+0x54*n+0x2c) = 0;
	}
	if(!strncmp(bakes, "files:", 6)){

#if defined(DR_IMA_TILES)
		fprintf(stderr, "replay-tiles can't set a race up from the game's files\n");
		exit(1);
#else
		void dRally_Memory_init(void);

		if(chdir(bakes+6)){ perror(bakes+6); exit(1); }
		dRally_Memory_init();		// race_alloc() allocates through it
		return &race_loaders_files;
#endif
	}
	race_baked = race_baked_read(bakes);

	return &race_loaders_baked;
}

// gba/race's quick race: models and colors as in its main.c
static int quick(char * argv[]){

	__BYTE__ * 		flat = malloc(race_replay_size());

	race_setup(0, 4, quick_setup(argv[2], argv[3], strtoul(argv[4], 0, 0)));
	race_replay_gather(flat);
	printf("set up: globals hash %08x\n", race_replay_hash(flat));
	if(getenv("VAR_HASHES")){

		unsigned 	i, o, k;
		__DWORD__ 	h;

		for(i = o = 0; i < race_replay_count(); o += race_replay_var_size(i++)){
			for(h = 0x811c9dc5, k = 0; k < race_replay_var_size(i); k++) h = (h^flat[o+k])*0x01000193;
			printf("var %s %08x\n", race_replay_var_name(i), h);
		}
	}

	return 0;
}

// a hash of the track image as the race code sees it
static __DWORD__ ima_hash(void){

	__DWORD__ 	h = 0x811c9dc5;
	__BYTE__ 	row[0x800];
	int 		x, y;

	for(y = 0; y < TRX_HEIGHT; y++){

		race_ima_read(0, y, row, TRX_WIDTH);
		for(x = 0; x < TRX_WIDTH; x++) h = (h^row[x])*0x01000193;
	}

	return h;
}

// gba/race's benchmark race: see its main.c
static int race(char * argv[]){

	__BYTE__ * 		flat = malloc(race_replay_size());
	struct_35e_t * 		cars = (struct_35e_t *)___1e6ed0h;
	unsigned 		ticks = 0, n;

	race_setup(0, 4, quick_setup(argv[2], argv[3], strtoul(argv[4], 0, 0)));
	race_replay_gather(flat);
	printf("set up: globals hash %08x\n", race_replay_hash(flat));

	D(___243ce0h) = 1;
	CURRENT_VIEWPORT_W = 0x140;
	CURRENT_VIEWPORT_X = 0;
	CURRENT_VIEWPORT_CENTER_X = CURRENT_VIEWPORT_W/2;
	race_render();

	while((int)D(___2438c8h) <= 0x12c){

		race___40164h();
		timer_ticks++;
		race_logic();
		race_render();
		race_render_after();
		ticks++;
	}

	race_replay_gather(flat);
	printf("%u ticks; globals hash %08x, track image %08x\n", ticks, race_replay_hash(flat), ima_hash());
	for(n = 0; n < 4; n++) printf("car %u: lap %d pos %d fin %u\n", n, cars[n].Lap, cars[n].Position, cars[n].Finished);
#if defined(DR_IMA_TILES)
	{
		unsigned 	used, max, dropped;

		race_ima_stats(&used, &max, &dropped);
		printf("track image: %u of %u tiles drawn into, %u writes dropped\n", used, max, dropped);
	}
#endif

	return 0;
}

// --- the career ------------------------------------------------------------------

	extern __BYTE__ ___1a01e0h[];
	extern __BYTE__ ___1a1ef8h[];
	extern __BYTE__ ___1a103ch[];
	extern char ___19bd64h[16];
	extern int NUM_OF_LAPS;
	extern int NUM_OF_CARS;
	extern __POINTER__ ___1a0fe0h;
	extern __POINTER__ ___1a0fb8h;
void srand_watcom106(unsigned int seed);
void ___3079ch_cdecl(__DWORD__);
void race___400ech(void);

static const char * career_bakes;
static int career_round;

// the career's race, as gba/race's start_race() and run_race() run it in
// its benchmark
static void career_race_run(int my_index, int num_cars){

	static race_baked_t * 	baked[10];
	__BYTE__ * 		flat = malloc(race_replay_size());
	struct_35e_t * 		cars = (struct_35e_t *)___1e6ed0h;
	int 			track = ___19bd64h[2]-'0', n;

	if(!baked[track]){

		char 	dir[1024];

		snprintf(dir, sizeof(dir), "%s/bake-tr%d", career_bakes, track);
		baked[track] = (race_baked_t *)race_baked_read(dir);
		baked[track]->variants = 7;	// gba/race's: no spiked cars
	}
	race_baked = baked[track];
	timer_ticks = 0;
	race_setup(my_index, num_cars, &race_loaders_baked);
	race_replay_gather(flat);
	printf("career race on %s%s, %d laps, you in place %d of %d; set up: globals hash %08x\n", ___19bd64h,
		D(___1de7d0h+0x48) ? " reversed" : "", NUM_OF_LAPS, my_index, num_cars, race_replay_hash(flat));
	D(___243ce0h) = 1;
	CURRENT_VIEWPORT_W = 0x140;
	CURRENT_VIEWPORT_X = 0;
	CURRENT_VIEWPORT_CENTER_X = CURRENT_VIEWPORT_W/2;
	race_render();
	while((int)D(___2438c8h) <= 0x12c){

		race___40164h();
		timer_ticks++;
		race_logic();
		race_render();
		race_render_after();
	}
	race_replay_gather(flat);
	printf("  end: globals hash %08x\n", race_replay_hash(flat));
	if(getenv("VAR_HASHES")){

		unsigned 	i, o, k;
		__DWORD__ 	h;

		for(i = o = 0; i < race_replay_count(); o += race_replay_var_size(i++)){
			for(h = 0x811c9dc5, k = 0; k < race_replay_var_size(i); k++) h = (h^flat[o+k])*0x01000193;
			printf("end var %s %08x\n", race_replay_var_name(i), h);
		}
	}
	for(n = 0; n < NUM_OF_CARS; n++) ((struct_94_t *)___1de580h)[n].__54 = cars[n].Position;
	race___400ech();
	free(flat);
}

static void career_round_end(int A1){

	racer_t * 	me;
	const __BYTE__ * 	p = ___1a01e0h;
	__DWORD__ 	h = 0x811c9dc5;
	unsigned 	n;

	career_results(A1);
	me = (racer_t *)___1a01e0h+D(___1a1ef8h);	// sorted again
	for(n = 0; n < 0x14*sizeof(racer_t); n++) h = (h^p[n])*0x01000193;
	printf("CAREER round %d: you rank %d with %d points, $%d; racers hash %08x\n", ++career_round, me->rank, me->points, me->money, h);
}

// a race of the round, as gba/race's career_ui.c race(): 1 if it ends the game
static int career_race(int num_cars){

	__BYTE__ 	grid[4];

	career_race_track(grid);
	career_race_fill(num_cars, grid);
	career_race_run(D(___1a103ch), num_cars);
	career_race_after(num_cars, grid);
	if(career_leading()){

		if(career_leader_round()) return 1;
		career_round_end(0);
	}
	else career_round_end(0);

	return 0;
}

static int career(char * argv[]){

	static __BYTE__ 	carcol[0x300], copper[0x300];
	char 		path[1024];
	int 		rounds = atoi(argv[4]);
	FILE * 		fd;

	career_bakes = argv[2];
	snprintf(path, sizeof(path), "%s/bake-tr0/carcol.pal", career_bakes);
	if(!(fd = fopen(path, "rb"))||(fread(carcol, 1, 0x300, fd) != 0x300)){ perror(path); return 1; }
	fclose(fd);
	snprintf(path, sizeof(path), "%s/bake-tr0/copper.pal", career_bakes);
	if(!(fd = fopen(path, "rb"))||(fread(copper, 1, 0x300, fd) != 0x300)){ perror(path); return 1; }
	fclose(fd);
	___1a0fb8h = copper;
	___1a0fe0h = carcol;
	srand_watcom106(strtoul(argv[3], 0, 0));
	career_new("RACER", 0, 1);
	// CAREER_LEAD=1: you lead from the start, as gba/race's -DCAREER_LEAD
	if(getenv("CAREER_LEAD")&&atoi(getenv("CAREER_LEAD"))) ((racer_t *)___1a01e0h+D(___1a1ef8h))->points = 200;

	while(career_round < rounds){

		if(career_leading()){

			career_adversary_accept();
			if(career_race(2)){

				printf("CAREER the Adversary beaten\n");
				break;
			}
			continue;
		}
		career_signup_open();
		if(career_signup_check(0) == CAREER_FULL){

			career_signup_rest();
			career_round_end(1);
			continue;
		}
		career_signup(0);
		while(!career_signup_full()) ___3079ch_cdecl(3);
		career_signup_order();
		if(career_race(4)) break;
	}

	return 0;
}

int main(int argc, char * argv[]){

	FILE * 		fd;
	long 		size;
	__DWORD__ * 	log;
	__DWORD__ * 	expected;
	__BYTE__ * 	flat;
	const char * 	err;
	unsigned 	frames = 0, mismatches = 0, first_bad = 0, finish[4] = { 0 };
	int 		i;
	struct_35e_t * 	cars = (struct_35e_t *)___1e6ed0h;
	clock_t 	t;

	if((argc >= 4)&&!strcmp(argv[1], "--setup")) return setup(argc, argv);
	if((argc == 5)&&!strcmp(argv[1], "--quick")) return quick(argv);
	if((argc == 5)&&!strcmp(argv[1], "--race")) return race(argv);
	if((argc == 5)&&!strcmp(argv[1], "--career")) return career(argv);
	if((argc != 2)&&(argc != 3)){ fprintf(stderr, "usage: replay TICKLOG.BIN [OUT.BIN]\n"); return 1; }

	if(!(fd = fopen(argv[1], "rb"))){ perror(argv[1]); return 1; }
	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	log = malloc(size);
	if(fread(log, 1, size, fd) != (size_t)size){ perror(argv[1]); return 1; }
	fclose(fd);

	flat = malloc(race_replay_size());
	if((err = race_replay_start(log, flat))){ fprintf(stderr, "%s: %s\n", argv[1], err); return 1; }

	// the logic draws into these, so they get copies: OUT.BIN keeps the recorded ones
	for(i = 3; i <= 5; i += 2){

		__POINTER__ 	p = malloc(race_replay_buf_size(i));

		memcpy(p, *race_replay_buf(i), race_replay_buf_size(i));
		*race_replay_buf(i) = p;
	}

	t = clock();
	while(race_replay_next()){

		__DWORD__ 	h;

		race_logic();
		expected = race_replay_done();
		h = race_replay_hash(flat);
		if(h != *expected){

			if(!mismatches) first_bad = frames;
			mismatches++;
			*expected = h;
		}
		frames++;
		for(i = 0; i < 4; i++) if(!finish[i]&&cars[i].Finished) finish[i] = frames;
	}
	t = clock()-t;

	// how the race came out, to compare a change to the math with the
	// original: the hashes can't, a race diverges from the first changed bit
	for(i = 0; i < 4; i++){

		printf("car %d: finished %5u, place %u, lap %u, damage %d, at %.0f,%.0f\n", i, finish[i], cars[i].Position, cars[i].Lap, (int)D(___1de580h+0x94*i+0x18), cars[i].XLocation, cars[i].YLocation);
	}

	printf("%u frames in %.2f s\n", frames, (double)t/CLOCKS_PER_SEC);

	// what the logic drew into the track, which the hashes leave out
	{
		__DWORD__ 	h = 0x811c9dc5;
		unsigned 	k;

		for(k = 0; k < race_replay_buf_size(3); k++) h = (h^(*race_replay_buf(3))[k])*0x01000193;
		printf("track image %08x\n", h);
	}
	if(mismatches) printf("%u frames differ from the recording, the first is frame %u\n", mismatches, first_bad);
	else printf("all hashes match the recording\n");

	if(argc == 3){

		if(!(fd = fopen(argv[2], "wb"))||(fwrite(log, 1, size, fd) != (size_t)size)||fclose(fd)){ perror(argv[2]); return 1; }
		printf("wrote %s\n", argv[2]);
	}

	return mismatches ? 2 : 0;
}
