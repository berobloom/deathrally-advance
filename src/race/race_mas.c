#include "drally.h"
#include "drally_race.h"

	extern int TRX_WIDTH;
	extern int TRX_HEIGHT;
	extern __POINTER__ TRX_MAS;

// The track's mask, TRX_MAS: under each pixel a byte whose low 4 bits the
// race code reads, below 4 a wall, 0xf nothing. The game reads it as
// TRX_MAS[TRX_WIDTH*y+x], so an x past a row's end runs on into the next
// row; off the track it reads past the buffer, which is 0xf here.

#if !defined(DR_MAS_BLOCKS)

int race_mas(int x, int y){

	int 	i = TRX_WIDTH*y+x;

	if((i < 0)||(i >= TRX_WIDTH*TRX_HEIGHT)) return 0xf;

	return TRX_MAS[i]&0xf;
}

void race_mas_row(int x, int y, int n, __BYTE__ * dst){

	int 	i = TRX_WIDTH*y+x, k, end = TRX_WIDTH*TRX_HEIGHT;

	for(k = 0; k < n; k++, i++) dst[k] = ((i < 0)||(i >= end)) ? 0xf : TRX_MAS[i]&0xf;
}

#else

// Built with DR_MAS_BLOCKS (the GBA, where a track's mask and the same
// turned round took 1.4 MB of ROM) it is gbabake's track.mbk: the track
// in 8x8 blocks, a row of them after another, each a u16:
//
//   0x8000|v   all of its pixels v
//   n          block n of the 4-bit ones after the map, 32 bytes each,
//              a row of 8 pixels in 4, the left one of 2 in the low bits
//
// after the counts of blocks across and down and of the 4-bit ones; after
// those, each one's walls, below 4, a byte a row, bit k pixel k. A reversed track (race___4af3ch())
// is the same read turned round.

static const __WORD__ * 	map;
static const __BYTE__ * 	blocks;
static const __BYTE__ * 	walls;
static int 			bw, bh, rev;

void race_mas_blocks(const __BYTE__ * mbk){

	bw = ((const __DWORD__ *)mbk)[0];
	bh = ((const __DWORD__ *)mbk)[1];
	map = (const __WORD__ *)(mbk+12);
	blocks = mbk+12+((2*bw*bh+3)&~3);
	walls = blocks+32*((const __DWORD__ *)mbk)[2];
	rev = 0;
}

void race_mas_reverse(void){ rev = 1; }

// n pixels of row y, as it's stored, from x on, to the right or, d -1,
// to the left
DR_HOT static void row_part(int x, int y, int n, int d, __BYTE__ * dst){

	const __WORD__ 	* m = map+bw*(y >> 3);
	int 		r = 4*(y&7);

	while(n > 0){

		unsigned 	e = m[x >> 3];
		int 		k = (d > 0) ? 8-(x&7) : (x&7)+1;

		if(k > n) k = n;
		n -= k;
		if(e&0x8000){

			e &= 0xf;
			x += d*k;
			while(k--) *dst++ = e;
		}
		else {

			const __BYTE__ 	* b = blocks+32*e+r;

			for(; k > 0; k--, x += d) *dst++ = (b[(x&7) >> 1] >> (4*(x&1)))&0xf;
		}
	}
}

DR_HOT int race_mas(int x, int y){

	unsigned 	e;

	while(x < 0){ x += TRX_WIDTH; y--; }
	while(x >= TRX_WIDTH){ x -= TRX_WIDTH; y++; }
	if((y < 0)||(y >= TRX_HEIGHT)) return 0xf;
	if(rev){ x = TRX_WIDTH-1-x; y = TRX_HEIGHT-1-y; }
	e = map[bw*(y >> 3)+(x >> 3)];
	if(e&0x8000) return e&0xf;

	return (blocks[32*e+4*(y&7)+((x&7) >> 1)] >> (4*(x&1)))&0xf;
}

DR_HOT void race_mas_row(int x, int y, int n, __BYTE__ * dst){

	while(n > 0){

		int 	k;

		// past a row's end, the next one's
		while(x < 0){ x += TRX_WIDTH; y--; }
		while(x >= TRX_WIDTH){ x -= TRX_WIDTH; y++; }
		if((y < 0)||(y >= TRX_HEIGHT)){

			*dst++ = 0xf;
			x++;
			n--;
			continue;
		}
		k = TRX_WIDTH-x;
		if(k > n) k = n;
		if(rev) row_part(TRX_WIDTH-1-x, TRX_HEIGHT-1-y, k, -1, dst);
		else row_part(x, y, k, 1, dst);
		dst += k;
		x += k;
		n -= k;
	}
}

// whether the 8x8 block (bx, by), as the race reads the mask, holds a
// wall, for race_walls_build(). Turned round, a block can straddle up to 4
// stored ones
int race_mas_wall_block(int bx, int by){

	int 	x0 = 8*bx, x1 = 8*bx+7, y0 = 8*by, y1 = 8*by+7, x, y;

	if(rev){

		x = TRX_WIDTH-1-x1; x1 = TRX_WIDTH-1-x0; x0 = x;
		y = TRX_HEIGHT-1-y1; y1 = TRX_HEIGHT-1-y0; y0 = y;
	}
	if(x0 < 0) x0 = 0;
	if(y0 < 0) y0 = 0;
	if(x1 >= TRX_WIDTH) x1 = TRX_WIDTH-1;
	if(y1 >= TRX_HEIGHT) y1 = TRX_HEIGHT-1;
	for(y = y0 >> 3; y <= y1 >> 3; y++){
		for(x = x0 >> 3; x <= x1 >> 3; x++){

			unsigned 	e = map[bw*y+x];
			int 		r0, r1, c0, c1, cols;

			if(e&0x8000){

				if((e&0xf) < 4) return 1;
				continue;
			}
			// its rows and columns inside the one asked for
			r0 = (y == y0 >> 3) ? y0&7 : 0;
			r1 = (y == y1 >> 3) ? y1&7 : 7;
			c0 = (x == x0 >> 3) ? x0&7 : 0;
			c1 = (x == x1 >> 3) ? x1&7 : 7;
			cols = (0xff << c0)&(0xff >> (7-c1));
			for(; r0 <= r1; r0++) if(walls[8*e+r0]&cols) return 1;
		}
	}

	return 0;
}

#endif // DR_MAS_BLOCKS
