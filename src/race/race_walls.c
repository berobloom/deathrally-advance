#include "drally.h"
#include "drally_race.h"

	extern int TRX_WIDTH;
	extern int TRX_HEIGHT;
	extern __POINTER__ TRX_MAS;
	extern __POINTER__ ___243d74h;

// Which 8x8 blocks of the track hold a wall: a mask value below 4. The
// race code tests the 40x40 pixels under a car against the mask every
// tick, so it asks here first and skips the test where no wall is near,
// which finds nothing the test wouldn't.

#define WALL_BLOCK	3

static __BYTE__ * 	walls;
static int 		walls_w, walls_h;

// and the box around each car image's solid pixels (above 3), 96 images
// of 40x40 for each of the 4 cars: first and last column, first and last
// row, or a first column of 40 if there are none
static __BYTE__ 	car_boxes[4*96][4];

// pixels read 4 at a time, which the GBA reads from ROM about as fast as one
typedef __DWORD__ __attribute__((may_alias)) px4_t;

// bit 0 of each byte of the result: whether that pixel is solid, above 3
static __DWORD__ solid4(__DWORD__ px){

	px &= 0xfcfcfcfc;
	px |= px >> 4;
	px |= px >> 2;

	return (px|(px >> 1)) & 0x01010101;
}

static void car_boxes_build(void){

	int 		n, x, y, x0, x1, y0, y1;
	const px4_t * 	img;
	__DWORD__ 	cols[10], row;

	for(n = 0; n < 4*96; n++){

		// the rows with solid pixels, and the columns, as bits
		img = (const px4_t *)(___243d74h+0x640*n);
		y0 = 0x28; y1 = 0;
		memset(cols, 0, sizeof(cols));

		for(y = 0; y < 0x28; y++){

			row = 0;
			for(x = 0; x < 10; x++){

				__DWORD__ 	s = solid4(img[10*y+x]);

				cols[x] |= s;
				row |= s;
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

		car_boxes[n][0] = x0; car_boxes[n][1] = x1; car_boxes[n][2] = y0; car_boxes[n][3] = y1;
	}
}

const __BYTE__ * race_car_box(__DWORD__ img_offset){

	return car_boxes[img_offset/0x640];
}

// after the track and the car images are loaded: race___42824h() decodes
// the images and race___4af3ch() mirrors them for reversed tracks, both
// before this
void race_walls_build(void){

	int 		x, y, w, h, ww;
	__BYTE__ * 	mas, * row;

	car_boxes_build();

	w = TRX_WIDTH;
	h = TRX_HEIGHT;
	walls_w = ww = (w+7) >> WALL_BLOCK;
	walls_h = (h+7) >> WALL_BLOCK;
	free(walls);
	walls = calloc(walls_w*walls_h, 1);

	mas = TRX_MAS;
	for(y = 0; y < h; y++){

		row = walls+ww*(y >> WALL_BLOCK);
		x = 0;
		// a mask value below 4 has bits 2 and 3 clear, 4 at a time where
		// the rows are word-aligned
		if(!(w&3)&&!((unsigned long)mas&3)){

			const px4_t * 	m = (const px4_t *)(mas+w*y);

			for(; x < w; x += 4){

				__DWORD__ 	px = *m++;

				if((((px >> 2)|(px >> 3)) & 0x01010101) != 0x01010101) row[x >> WALL_BLOCK] = 1;
			}
		}
		for(; x < w; x++){

			if((mas[w*y+x]&0xf) < 4) row[x >> WALL_BLOCK] = 1;
		}
	}
}

void race_walls_free(void){

	free(walls);
	walls = 0;
}

// 0 if the w x h pixels from (x, y) on hold no wall. Areas that reach past
// the track's edges get 1: the race code reads those from the next or the
// previous row of the mask, so they need the full test
DR_HOT int race_walls_near(int x, int y, int w, int h){

	int 	bx, by;

	if((x < 0)||(y < 0)||(x+w > TRX_WIDTH)||(y+h > TRX_HEIGHT)) return 1;

	for(by = y >> WALL_BLOCK; by <= (y+h-1) >> WALL_BLOCK; by++){
		for(bx = x >> WALL_BLOCK; bx <= (x+w-1) >> WALL_BLOCK; bx++){

			if(walls[walls_w*by+bx]) return 1;
		}
	}

	return 0;
}

// which of the 8x8 blocks of the wall map that the w x h pixels from
// (x, y) on reach into, column by column from x >> 3, hold a wall: bit k
// for column (x >> 3)+k, all of them where the area isn't on the track
DR_HOT unsigned race_walls_cells(int x, int y, int w, int h){

	int 		bx, by;
	unsigned 	cells = 0;

	if((x < 0)||(y < 0)||(x+w > TRX_WIDTH)||(y+h > TRX_HEIGHT)) return ~0u;

	for(by = y >> WALL_BLOCK; by <= (y+h-1) >> WALL_BLOCK; by++){
		for(bx = x >> WALL_BLOCK; bx <= (x+w-1) >> WALL_BLOCK; bx++){

			if(walls[walls_w*by+bx]) cells |= 1u << (bx-(x >> WALL_BLOCK));
		}
	}

	return cells;
}
