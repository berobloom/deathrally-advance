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

#include "drally.h"
#include "drally_structs_free.h"
#include "race_replay.h"
#include "drally_race.h"

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
// interrupt, as in gba/race: no keys, the AI drives
__BYTE__ kmap[256];
__DWORD__ __GET_FRAME_COUNTER(void){ return 0; }

// sound isn't replayed
void dRally_Sound_pushEffect(__BYTE__ channel, __BYTE__ n, __DWORD__ unk, __DWORD__ a0, __DWORD__ a1, __DWORD__ a2){}
void dRally_Sound_freeEffectChannel(__BYTE__ ch_num){}
void dRally_Sound_adjustEffect(__BYTE__ channel, __DWORD__ vol, __DWORD__ freq, __DWORD__ balance){}

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
	if(D(___1de7d0h+0x48) != 0){ fprintf(stderr, "%s: a reversed track, which isn't baked yet\n", argv[2]); return 1; }
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

// gba/race's quick race: models and colors as in its main.c
static int quick(char * argv[]){

	static const int 	models[4] = { 3, 5, 4, 2 };
	static const __BYTE__ 	colors[4][3] = { {63, 8, 4}, {8, 24, 63}, {63, 52, 4}, {8, 52, 12} };
	__BYTE__ * 		flat = malloc(race_replay_size());

	race_quick(atoi(argv[2]), models, colors, strtoul(argv[4], 0, 0));
	race_baked = race_baked_read(argv[3]);
	race_setup(0, 4, &race_loaders_baked);
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

	static const int 	models[4] = { 3, 5, 4, 2 };
	static const __BYTE__ 	colors[4][3] = { {63, 8, 4}, {8, 24, 63}, {63, 52, 4}, {8, 52, 12} };
	__BYTE__ * 		flat = malloc(race_replay_size());
	struct_35e_t * 		cars = (struct_35e_t *)___1e6ed0h;
	unsigned 		ticks = 0, n;

	race_quick(atoi(argv[2]), models, colors, strtoul(argv[4], 0, 0));
	race_baked = race_baked_read(argv[3]);
	race_setup(0, 4, &race_loaders_baked);
	race_replay_gather(flat);
	printf("set up: globals hash %08x\n", race_replay_hash(flat));

	D(___243ce0h) = 1;
	CURRENT_VIEWPORT_W = 0x140;
	CURRENT_VIEWPORT_X = 0;
	CURRENT_VIEWPORT_CENTER_X = CURRENT_VIEWPORT_W/2;
	race_render();

	while((int)D(___2438c8h) <= 0x12c){

		race___40164h();
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
