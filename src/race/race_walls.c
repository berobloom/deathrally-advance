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

static void car_boxes_build(void){

	int 		n, x, y;
	__BYTE__ * 	img;

	for(n = 0; n < 4*96; n++){

		__BYTE__ * 	b = car_boxes[n];

		img = ___243d74h+0x640*n;
		b[0] = 0x28; b[1] = 0; b[2] = 0x28; b[3] = 0;

		for(y = 0; y < 0x28; y++){
			for(x = 0; x < 0x28; x++){

				if(img[0x28*y+x] > 3){

					if(x < b[0]) b[0] = x;
					if(x > b[1]) b[1] = x;
					if(y < b[2]) b[2] = y;
					if(y > b[3]) b[3] = y;
				}
			}
		}
	}
}

const __BYTE__ * race_car_box(__DWORD__ img_offset){

	return car_boxes[img_offset/0x640];
}

// after the track and the car images are loaded: race___42824h() decodes
// the images and race___4af3ch() mirrors them for reversed tracks, both
// before this
void race_walls_build(void){

	int 	x, y;

	car_boxes_build();

	walls_w = (TRX_WIDTH+7) >> WALL_BLOCK;
	walls_h = (TRX_HEIGHT+7) >> WALL_BLOCK;
	free(walls);
	walls = calloc(walls_w*walls_h, 1);

	for(y = 0; y < TRX_HEIGHT; y++){
		for(x = 0; x < TRX_WIDTH; x++){

			if((B(TRX_MAS+TRX_WIDTH*y+x)&0xf) < 4) walls[walls_w*(y >> WALL_BLOCK)+(x >> WALL_BLOCK)] = 1;
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
