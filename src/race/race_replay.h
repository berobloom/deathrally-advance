#ifndef __RACE_REPLAY_H
#define __RACE_REPLAY_H

#include "types.h"

// Replaying race_logic(): the desktop records the globals it uses at the
// start of a race, then for every frame what changed in them before the
// logic ran (input, the timer, rendering) and a hash after it. Another
// build of the logic (the GBA's) replays the changes and compares hashes.
//
// TICKLOG.BIN, all little-endian 32-bit words:
//   "DRTL", version, size of the globals, number of globals, their sizes
//   8 buffers, each a size and its bytes padded to 4, in race_replay_bufs order
//   the globals at the first frame
//   then per frame: number of runs, each an offset, a length and its bytes
//   padded to 4, then the hash after race_logic(); 0xffffffff ends the file

#define RACE_REPLAY_MAGIC	0x4c545244	// "DRTL"
#define RACE_REPLAY_VERSION	3
#define RACE_REPLAY_END		0xffffffff
#define RACE_REPLAY_NBUFS	8

unsigned race_replay_size(void);
unsigned race_replay_count(void);
unsigned race_replay_var_size(unsigned i);
void race_replay_gather(__BYTE__ * flat);
void race_replay_scatter(const __BYTE__ * flat);
__DWORD__ race_replay_hash(const __BYTE__ * flat);

// the track and sprite buffers race_logic() reads through pointers:
// TRX_MAS, TRX_VAI, TRX_LR1, TRX_IMA, the car images, obstacles, mines, blasts
__POINTER__ * race_replay_buf(unsigned i);
unsigned race_replay_buf_size(unsigned i);

const char * race_replay_start(__DWORD__ * log, __BYTE__ * flat);
int race_replay_next(void);
__DWORD__ * race_replay_done(void);

void race_ticklog_before(void);
void race_ticklog_after(void);
void race_ticklog_end(void);
void race_check_nodraw(void);

#endif // __RACE_REPLAY_H
