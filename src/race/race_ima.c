#include <stdlib.h>
#include <string.h>
#include "drally.h"
#include "drally_race.h"

	extern __POINTER__ TRX_IMA;
	extern int TRX_WIDTH;
	extern int TRX_HEIGHT;

// The track image, which the race code draws skid marks, mines, craters,
// power-ups and obstacles into, and reads the pixels under them back from.
// Its pixels are addressed as they are in TRX_IMA: a row that runs past
// the image's right edge goes on at the left of the next row.

#if !defined(DR_IMA_TILES)

void race_ima_read(int x, int y, __BYTE__ * dst, int n){

	memcpy(dst, TRX_IMA+TRX_WIDTH*y+x, n);
}

void race_ima_write(int x, int y, const __BYTE__ * src, int n){

	memcpy(TRX_IMA+TRX_WIDTH*y+x, src, n);
}

void race_ima_draw(int x, int y, const __BYTE__ * src, int n){

	__BYTE__ * 	dst = TRX_IMA+TRX_WIDTH*y+x;
	int 		i;

	for(i = 0; i < n; i++) if(src[i] != 0) dst[i] = src[i];
}

void race_ima_lookup(int x, int y, int w, int h, const __BYTE__ * tab){

	__BYTE__ * 	dst = TRX_IMA+TRX_WIDTH*y+x;
	int 		i, j;

	for(j = 0; j < h; j++, dst += TRX_WIDTH) for(i = 0; i < w; i++) dst[i] = tab[dst[i]];
}

#else

// The GBA has no room for the whole image: it is the track's 8x8 tiles in
// ROM, with the ones drawn into copied to a pool in RAM. A tile that
// changes goes on a list for the screen to load again.

static const __BYTE__ * 	ima_tiles;	// 64 bytes each, row by row
static int 			ima_tw, ima_th;
static __BYTE__ 		(*pool)[64];
static unsigned 		pool_n, pool_max, pool_full;
// each tile's place in the pool+1, or 0; bit 15: on the list, which the
// screen clears from the vblank interrupt
static volatile unsigned short * 	slots;

#define DIRTY_N 	512
static unsigned short 		dirty[DIRTY_N];	// ty << 8 | tx
static volatile unsigned 	dirty_head, dirty_tail;
static volatile int 		dirty_lost;

void race_ima_tiles(const __BYTE__ * tiles, int tw, int th, unsigned max_tiles){

	ima_tiles = tiles;
	ima_tw = tw;
	ima_th = th;
	free((void *)slots);
	slots = calloc(tw*th, sizeof(slots[0]));
	if(max_tiles != pool_max){

		free(pool);
		pool = malloc(max_tiles*64);
		pool_max = pool ? max_tiles : 0;
	}
	pool_n = pool_full = 0;
	dirty_tail = dirty_head;
	dirty_lost = 0;
}

const __BYTE__ * race_ima_tile(int tx, int ty){

	unsigned 	s = slots[ty*ima_tw+tx]&0x7fff;

	return s ? pool[s-1] : ima_tiles+64*(ty*ima_tw+tx);
}

int race_ima_dirty(void){

	unsigned 	t;

	if(dirty_lost){

		// too many at once: all of them
		dirty_lost = 0;
		dirty_tail = dirty_head;
		for(t = 0; t < (unsigned)(ima_tw*ima_th); t++) slots[t] &= 0x7fff;

		return -2;
	}
	if(dirty_tail == dirty_head) return -1;
	t = dirty[dirty_tail%DIRTY_N];
	slots[(t >> 8)*ima_tw+(t&0xff)] &= 0x7fff;
	dirty_tail++;

	return t;
}

void race_ima_stats(unsigned * used, unsigned * max, unsigned * dropped){

	*used = pool_n;
	*max = pool_max;
	*dropped = pool_full;
}

// pixel (x, y) where the row before or after it holds it, or 0 if it's
// outside the image
static inline int place(int * x, int * y){

	while(*x < 0){ *x += TRX_WIDTH; (*y)--; }
	while(*x >= TRX_WIDTH){ *x -= TRX_WIDTH; (*y)++; }

	return (*y >= 0)&&(*y < TRX_HEIGHT);
}

// after drawing into tile t (tx, ty): the screen needs it again
static inline void changed(unsigned t, int tx, int ty){

	slots[t] |= 0x8000;
	if(dirty_head-dirty_tail >= DIRTY_N){

		dirty_lost = 1;
		return;
	}
	dirty[dirty_head%DIRTY_N] = (ty << 8)|tx;
	dirty_head++;
}

// tile t copied to the pool, from p, to be drawn into, or 0 if the pool is full
static inline __BYTE__ * to_pool(unsigned t, const __BYTE__ * p){

	__DWORD__ * 		d;
	const __DWORD__ * 	q = (const __DWORD__ *)p;
	int 			i;

	if(pool_n == pool_max){

		pool_full++;
		return 0;
	}
	d = (__DWORD__ *)pool[pool_n];
	for(i = 0; i < 16; i++) d[i] = q[i];
	slots[t] |= ++pool_n;

	return pool[pool_n-1];
}

// what span() does with the n pixels from (x, y)
#define OP_READ 	0
#define OP_WRITE 	1	// all of buf's
#define OP_DRAW 	2	// buf's non-zero ones

// k pixels from o in tile t (tx, ty). Only a tile that changes is copied
// to the pool and goes on the list: putting back the pixels under an
// obstacle, or drawing it again over itself, often doesn't
static inline void tile_op(int op, unsigned t, int tx, int ty, int o, __BYTE__ * buf, int k){

	unsigned 	s = slots[t];
	__BYTE__ * 	p = (s&0x7fff) ? pool[(s&0x7fff)-1] : (__BYTE__ *)ima_tiles+64*t;
	int 		i;

	p += o;
	if(op == OP_READ){

		for(i = 0; i < k; i++) buf[i] = p[i];
		return;
	}
	for(i = 0; i < k; i++) if(((op == OP_WRITE)||buf[i])&&(p[i] != buf[i])) break;
	if(i == k) return;
	if(!(s&0x7fff)){

		if(!(p = to_pool(t, p-o))) return;
		p += o;
	}
	for(; i < k; i++) if((op == OP_WRITE)||buf[i]) p[i] = buf[i];
	// read again: the screen may have taken it off the list meanwhile
	if(!(slots[t]&0x8000)) changed(t, tx, ty);
}

DR_HOT static void span(int op, int x, int y, __BYTE__ * buf, int n){

	int 	tx, k;
	unsigned 	t;

	// all in one row of the image: a tile at a time
	if((y >= 0)&&(y < TRX_HEIGHT)&&(x >= 0)&&(x+n <= TRX_WIDTH)){

		tx = x >> 3;
		t = (y >> 3)*ima_tw+tx;
		for(x &= 7; n > 0; x = 0, t++, tx++){

			k = 8-x;
			if(k > n) k = n;
			tile_op(op, t, tx, y >> 3, 8*(y&7)+x, buf, k);
			buf += k;
			n -= k;
		}
		return;
	}

	// running on into the next row, or outside the image: read as 0, not drawn
	while(n > 0){

		if(!place(&x, &y)){

			k = 1;
			if(op == OP_READ) *buf = 0;
		}
		else {

			k = 8-(x&7);
			if(k > n) k = n;
			if(k > TRX_WIDTH-x) k = TRX_WIDTH-x;
			tile_op(op, (y >> 3)*ima_tw+(x >> 3), x >> 3, y >> 3, 8*(y&7)+(x&7), buf, k);
		}
		x += k;
		buf += k;
		n -= k;
	}
}

void race_ima_read(int x, int y, __BYTE__ * dst, int n){ span(OP_READ, x, y, dst, n); }
void race_ima_write(int x, int y, const __BYTE__ * src, int n){ span(OP_WRITE, x, y, (__BYTE__ *)src, n); }
void race_ima_draw(int x, int y, const __BYTE__ * src, int n){ span(OP_DRAW, x, y, (__BYTE__ *)src, n); }

DR_HOT void race_ima_lookup(int x, int y, int w, int h, const __BYTE__ * tab){

	int 	i, j;

	for(j = 0; j < h; j++){
		for(i = 0; i < w; i++){

			int 		px = x+i, py = y+j;
			unsigned 	t, s;
			__BYTE__ * 	p;
			__BYTE__ 	v;

			if(((unsigned)px >= (unsigned)TRX_WIDTH)||((unsigned)py >= (unsigned)TRX_HEIGHT)){

				if(!place(&px, &py)) continue;
			}
			t = (py >> 3)*ima_tw+(px >> 3);
			s = slots[t];
			p = (s&0x7fff) ? pool[(s&0x7fff)-1] : (__BYTE__ *)ima_tiles+64*t;
			v = tab[p[8*(py&7)+(px&7)]];
			if(v == p[8*(py&7)+(px&7)]) continue;
			if(!(s&0x7fff)&&!(p = to_pool(t, p))) continue;
			p[8*(py&7)+(px&7)] = v;
			if(!(slots[t]&0x8000)) changed(t, px >> 3, py >> 3);
		}
	}
}

#endif // DR_IMA_TILES
