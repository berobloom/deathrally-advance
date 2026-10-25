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

#include "drally.h"
#include "drally_structs_free.h"
#include "race_replay.h"

	extern __BYTE__ ___1e6ed0h[];
	extern __BYTE__ ___1de580h[];

void race_logic(void);

// sound isn't replayed
void dRally_Sound_pushEffect(__BYTE__ channel, __BYTE__ n, __DWORD__ unk, __DWORD__ a0, __DWORD__ a1, __DWORD__ a2){}
void dRally_Sound_freeEffectChannel(__BYTE__ ch_num){}
void dRally_Sound_adjustEffect(__BYTE__ channel, __DWORD__ vol, __DWORD__ freq, __DWORD__ balance){}

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
	if(mismatches) printf("%u frames differ from the recording, the first is frame %u\n", mismatches, first_bad);
	else printf("all hashes match the recording\n");

	if(argc == 3){

		if(!(fd = fopen(argv[2], "wb"))||(fwrite(log, 1, size, fd) != (size_t)size)||fclose(fd)){ perror(argv[2]); return 1; }
		printf("wrote %s\n", argv[2]);
	}

	return mismatches ? 2 : 0;
}
