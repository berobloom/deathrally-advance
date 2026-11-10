#include <string.h>
#include "drally.h"
#include "drally_race.h"

// pixels read 4 at a time, which the GBA reads from ROM about as fast as one
typedef __DWORD__ __attribute__((may_alias)) px4_t;

// bit 0 of each byte of the result: whether that pixel is solid, above 3
static __DWORD__ solid4(__DWORD__ px){

	px &= 0xfcfcfcfc;
	px |= px >> 4;
	px |= px >> 2;

	return (px|(px >> 1)) & 0x01010101;
}

// the shapes of a car's 96 images of 40x40 (spins, word-aligned). Also
// built by tools/gbabake, so that the GBA needn't read the images at the
// start of every race
void race_car_shapes(const __BYTE__ * spins, race_car_shapes_t * out){

	int 		n, x, y, x0, x1, y0, y1, r0, r1;
	const px4_t * 	img;
	__DWORD__ 	cols[10], row, lo, hi, s;
	unsigned long long 	bits;	// bit x: pixel x of the row is solid

	for(n = 0; n < 96; n++){

		// the rows with solid pixels, and the columns, as bits
		img = (const px4_t *)(spins+0x640*n);
		y0 = 0x28; y1 = 0;
		memset(cols, 0, sizeof(cols));

		for(y = 0; y < 0x28; y++){

			row = lo = hi = 0;
			for(x = 0; x < 10; x++){

				s = solid4(img[10*y+x]);
				cols[x] |= s;
				row |= s;
				// the 4 pixels' bits (0, 8, 16, 24) to bits 24-27 by a multiply
				if(x < 8) lo |= ((s*0x01020408) >> 24) << (4*x);
				else hi |= ((s*0x01020408) >> 24) << (4*x-32);
			}
			bits = ((unsigned long long)hi << 32)|lo;

			if(bits){

				r0 = __builtin_ctzll(bits);
				r1 = 63-__builtin_clzll(bits);
				out->rows[n][y][0] = r0|((bits >> r0 == (2ull << (r1-r0))-1) ? 0x80 : 0);
				out->rows[n][y][1] = r1;
			}
			else {

				out->rows[n][y][0] = 0x28;
				out->rows[n][y][1] = 0;
			}
			if(row){

				if(y < y0) y0 = y;
				y1 = y;
			}
		}

		x0 = 0x28; x1 = 0;
		for(x = 0; x < 0x28; x++){

			if((cols[x >> 2] >> (8*(x&3)))&1){

				if(x < x0) x0 = x;
				x1 = x;
			}
		}

		out->box[n][0] = x0; out->box[n][1] = x1; out->box[n][2] = y0; out->box[n][3] = y1;
	}
}
