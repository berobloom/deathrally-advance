#include "drally.h"
#include "drally_structs_free.h"
#include "race_replay.h"
#include "drally_race.h"

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
	unsigned 	i, k, o, n = race_replay_size(), obst = 0;

	for(i = 0; vars[i].p != rv____1f2488h; i++) obst += vars[i].size;

	for(i = 0; i < obst; i += 4) h = (h^D(flat+i))*0x01000193;
	for(k = 0; k < 20; k++, i += 0x120){

		for(o = 0; o < 0x20; o += 4) h = (h^D(flat+i+o))*0x01000193;
	}
	for(; i < n; i += 4) h = (h^D(flat+i))*0x01000193;

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

#if defined(DR_REPLAY)

static __DWORD__ * 	play_p;
static __BYTE__ * 	play_flat;

// checks a recording and points the buffers into it. The caller gives
// TRX_IMA and the obstacles (buffers 3 and 5) writable memory if the
// recording isn't. Returns 0, or what is wrong.
const char * race_replay_start(__DWORD__ * log, __BYTE__ * flat){

	static char 	err[80];
	unsigned 	i, n;

	if((log[0] != RACE_REPLAY_MAGIC)||(log[1] != RACE_REPLAY_VERSION)) return "not a recording of this version";

	n = log[2];
	if((n != race_replay_size())||(log[3] != race_replay_count())){

		snprintf(err, sizeof(err), "%u bytes in %u globals, this build has %u in %u", n, log[3], race_replay_size(), race_replay_count());
		return err;
	}
	for(i = 0; i < log[3]; i++){

		if(log[4+i] != race_replay_var_size(i)){

			snprintf(err, sizeof(err), "global %u has %u bytes, %u in this build", i, log[4+i], race_replay_var_size(i));
			return err;
		}
	}
	log += 4+log[3];

	for(i = 0; i < RACE_REPLAY_NBUFS; i++){

		__DWORD__ 	size = *log++;

		*race_replay_buf(i) = (__POINTER__)log;
		log += (size+3)/4;
	}

	// D() loads whole words in the logic built with DR_ALIGNED (see
	// aligned.mk), which C doesn't promise for the __BYTE__ arrays
	for(i = 0; i < race_replay_count(); i++){

		if(!(vars[i].size&3)&&((unsigned long)vars[i].p&3)){

			snprintf(err, sizeof(err), "global %u isn't word-aligned", i);
			return err;
		}
	}

	memcpy(flat, log, n);
	race_replay_scatter(flat);	// so that race_replay_buf_size() works
	race_car_slots_contiguous();
	race_walls_build();
	play_p = log+(n+3)/4;
	play_flat = flat;

	return 0;
}

// applies what changed before the next frame's logic; 0 at the end
int race_replay_next(void){

	unsigned 	k, nruns;

	if((nruns = *play_p) == RACE_REPLAY_END) return 0;

	play_p++;
	for(k = 0; k < nruns; k++){

		__DWORD__ 	off = *play_p++, len = *play_p++;

		memcpy(play_flat+off, play_p, len);
		play_p += (len+3)/4;
	}
	race_replay_scatter(play_flat);

	return 1;
}

// after race_logic(): gathers the globals and returns the recorded hash's
// word, to compare with race_replay_hash(flat) (or to replace it)
__DWORD__ * race_replay_done(void){

	race_replay_gather(play_flat);

	return play_p++;
}

#endif // DR_REPLAY

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

#include <unistd.h>
#include <sys/wait.h>

	extern int dr_frame_counter_frozen;
	extern int TRX_VIEWPORT_TL_X, TRX_VIEWPORT_TL_Y;
	extern int CURRENT_VIEWPORT_W, CURRENT_VIEWPORT_X, CURRENT_VIEWPORT_CENTER_X;
	extern int X___243c8ch, Y___243c90h;

// with DR_CHECK_NODRAW set: race_render() with race_draw = 0, in a child
// process, against race_render() as usual. Both must leave the globals
// race_logic() uses, and the camera, the same: then a port can run the
// render functions without their drawing, as the GBA does, and get the
// same race. Time stands still meanwhile, so both see the same ticks
void race_check_nodraw(void){

	static unsigned 	frame, bad;
	unsigned 		i, n = race_replay_size(), first = ~0u, diffs = 0;
	__BYTE__ * 		a = malloc(n+0x20);
	__BYTE__ * 		b = malloc(n+0x20);
	int 			fd[2];
	pid_t 			pid;

	dr_frame_counter_frozen = 1;

	if(pipe(fd)||((pid = fork()) < 0)){ perror("race_check_nodraw"); exit(1); }

	if(pid == 0){

		race_draw = 0;
		race_render();
		race_replay_gather(a);
		memcpy(a+n, (int []){ TRX_VIEWPORT_TL_X, TRX_VIEWPORT_TL_Y, CURRENT_VIEWPORT_W, CURRENT_VIEWPORT_X, CURRENT_VIEWPORT_CENTER_X, X___243c8ch, Y___243c90h, 0 }, 0x20);
		if(write(fd[1], a, n+0x20) != (ssize_t)(n+0x20)) _exit(1);
		_exit(0);
	}

	race_render();
	race_replay_gather(b);
	memcpy(b+n, (int []){ TRX_VIEWPORT_TL_X, TRX_VIEWPORT_TL_Y, CURRENT_VIEWPORT_W, CURRENT_VIEWPORT_X, CURRENT_VIEWPORT_CENTER_X, X___243c8ch, Y___243c90h, 0 }, 0x20);

	close(fd[1]);
	for(i = 0; i < n+0x20; ){

		ssize_t 	r = read(fd[0], a+i, n+0x20-i);

		if(r <= 0){ fprintf(stderr, "race_check_nodraw: no state from the child\n"); exit(1); }
		i += r;
	}
	close(fd[0]);
	waitpid(pid, 0, 0);

	for(i = 0; i < n+0x20; i++){

		if(a[i] != b[i]){ if(first == ~0u) first = i; diffs++; }
	}
	if(diffs){

		bad++;
		fprintf(stderr, "frame %u: %u bytes differ without drawing, the first at %u of %u\n", frame, diffs, first, n);
	}
	if((frame%1000) == 0) fprintf(stderr, "race_check_nodraw: frame %u, %u frames differ\n", frame, bad);
	frame++;

	dr_frame_counter_frozen = 0;
	free(a);
	free(b);
}

#endif // DR_TICKLOG

#endif // DR_TICKLOG||DR_REPLAY
