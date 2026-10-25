#include "drally.h"
#include "drally_structs_free.h"
#include "race_replay.h"

// only in builds that record (DR_TICKLOG) or replay (DR_REPLAY) races: the
// externs below give globals other types than their definitions
#if defined(DR_TICKLOG)||defined(DR_REPLAY)

#define STR_(s)	#s
#define STR(s)	STR_(s)

// declared under their own names so the types the rest of the code gives
// them don't matter here
#define RV(n, s)	extern __BYTE__ rv_##n[] __asm__(STR(__USER_LABEL_PREFIX__) #n);
#include "race_replay_vars.h"
#undef RV

static const struct { __BYTE__ * p; unsigned size; } vars[] = {
#define RV(n, s)	{ rv_##n, s },
#include "race_replay_vars.h"
#undef RV
};

	extern int TRX_WIDTH;
	extern int TRX_HEIGHT;
	extern int TRX_WIDTH_QTR;
	extern int TRX_HEIGHT_QTR;
	extern __POINTER__ TRX_IMA;
	extern __POINTER__ TRX_MAS;
	extern __POINTER__ TRX_VAI;
	extern __POINTER__ TRX_LR1;
	extern car_spins_t * ___243d74h;
	extern __POINTER__ RACE_OBSTACLE_BPK;
	extern __POINTER__ RACE_MINES1A_BPK;
	extern __POINTER__ RACE_BLOWI_BPK;

unsigned race_replay_count(void){

	return sizeof(vars)/sizeof(vars[0]);
}

unsigned race_replay_var_size(unsigned i){

	return vars[i].size;
}

unsigned race_replay_size(void){

	unsigned 	i, n = 0;

	for(i = 0; i < race_replay_count(); i++) n += vars[i].size;

	return n;
}

void race_replay_gather(__BYTE__ * flat){

	unsigned 	i;

	for(i = 0; i < race_replay_count(); i++){

		memcpy(flat, vars[i].p, vars[i].size);
		flat += vars[i].size;
	}
}

void race_replay_scatter(const __BYTE__ * flat){

	unsigned 	i;

	for(i = 0; i < race_replay_count(); i++){

		memcpy(vars[i].p, flat, vars[i].size);
		flat += vars[i].size;
	}
}

// FNV-1a over 32-bit words, the globals' sizes are all multiples of 4. It
// leaves out the track pixels race___5209ch() keeps under each of the 20
// obstacles (0x20-0x11f of their 0x120 bytes at ___1f2488h): they are part
// of TRX_IMA, which the logic only draws into
__DWORD__ race_replay_hash(const __BYTE__ * flat){

	__DWORD__ 	h = 0x811c9dc5;
	unsigned 	i, o, n = race_replay_size(), obst = 0;

	for(i = 0; vars[i].p != rv____1f2488h; i++) obst += vars[i].size;

	for(i = 0; i < n; i += 4){

		o = i-obst;
		if((i >= obst)&&(o < 20*0x120)&&(o%0x120 >= 0x20)) continue;
		h = (h^D(flat+i))*0x01000193;
	}

	return h;
}

__POINTER__ * race_replay_buf(unsigned i){

	switch(i){
	case 0: return &TRX_MAS;
	case 1: return &TRX_VAI;
	case 2: return &TRX_LR1;
	case 3: return &TRX_IMA;
	case 4: return (__POINTER__ *)&___243d74h;
	case 5: return &RACE_OBSTACLE_BPK;
	case 6: return &RACE_MINES1A_BPK;
	case 7: return &RACE_BLOWI_BPK;
	default: return 0;
	}
}

// the sizes race_alloc() gives them
unsigned race_replay_buf_size(unsigned i){

	switch(i){
	case 0: case 3: return TRX_WIDTH*TRX_HEIGHT;
	case 1: case 2: return TRX_WIDTH_QTR*TRX_HEIGHT_QTR;
	case 4: return 4*sizeof(car_spins_t);
	case 5: return 0x800;
	case 6: return 0x140;
	case 7: return 0x600;
	default: return 0;
	}
}

#if defined(DR_TICKLOG)

	extern __BYTE__ ___243ce0h[];

static FILE * 		log_fd;
static FILE * 		post_fd;	// with DR_TICKLOG_POST set, every frame's globals after race_logic()
static int 		log_done;
static __BYTE__ * 	log_prev;
static __BYTE__ * 	log_cur;

static void put32(__DWORD__ v){

	fputc(v, log_fd); fputc(v >> 8, log_fd); fputc(v >> 16, log_fd); fputc(v >> 24, log_fd);
}

static void put_bytes(const __BYTE__ * p, unsigned n){

	fwrite(p, 1, n, log_fd);
	while(n++&3) fputc(0, log_fd);
}

// records the first race after start-up, with the AI driving every car
void race_ticklog_before(void){

	unsigned 	i, n, run, end;

	if(log_done) return;

	n = race_replay_size();

	if(!log_fd){

		if(!(log_fd = fopen("TICKLOG.BIN", "wb"))){ log_done = 1; return; }

		if(getenv("DR_TICKLOG_POST")) post_fd = fopen("TICKPOST.BIN", "wb");
		D(___243ce0h) = 1;
		log_prev = malloc(n);
		log_cur = malloc(n);

		put32(RACE_REPLAY_MAGIC);
		put32(RACE_REPLAY_VERSION);
		put32(n);
		put32(race_replay_count());
		for(i = 0; i < race_replay_count(); i++) put32(vars[i].size);
		for(i = 0; i < RACE_REPLAY_NBUFS; i++){

			put32(race_replay_buf_size(i));
			put_bytes(*race_replay_buf(i), race_replay_buf_size(i));
		}
		race_replay_gather(log_prev);
		put_bytes(log_prev, n);
	}

	race_replay_gather(log_cur);

	// runs of changed bytes, joined when less than 8 bytes apart
	run = 0;
	for(i = 0; i < n; i++){

		if(log_cur[i] == log_prev[i]) continue;
		end = i+1;
		while((end < n)&&((log_cur[end] != log_prev[end])||((end+8 < n)&&memcmp(log_cur+end, log_prev+end, 8)))) end++;
		run++;
		i = end;
	}
	put32(run);
	for(i = 0; i < n; i++){

		if(log_cur[i] == log_prev[i]) continue;
		end = i+1;
		while((end < n)&&((log_cur[end] != log_prev[end])||((end+8 < n)&&memcmp(log_cur+end, log_prev+end, 8)))) end++;
		put32(i);
		put32(end-i);
		put_bytes(log_cur+i, end-i);
		i = end;
	}
}

void race_ticklog_after(void){

	if(log_done||!log_fd) return;

	race_replay_gather(log_prev);
	put32(race_replay_hash(log_prev));
	if(post_fd) fwrite(log_prev, 1, race_replay_size(), post_fd);
}

void race_ticklog_end(void){

	if(log_done||!log_fd) return;

	put32(RACE_REPLAY_END);
	fclose(log_fd);
	if(post_fd) fclose(post_fd);
	free(log_prev);
	free(log_cur);
	log_done = 1;
}

#endif // DR_TICKLOG

#endif // DR_TICKLOG||DR_REPLAY
