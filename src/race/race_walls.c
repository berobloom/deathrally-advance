#include "drally.h"
#include "drmath.h"
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

// pixels read 4 at a time, which the GBA reads from ROM about as fast as one
typedef __DWORD__ __attribute__((may_alias)) px4_t;

// each car's shapes: baked ones, or built here into car_shapes
const race_car_shapes_t * 	race_car_shape[4];
static race_car_shapes_t * 	car_shapes;

__BYTE__ * race_car_slot[4];

void race_car_slots_contiguous(void){

	int 	n;

	for(n = 0; n < 4; n++){

		race_car_slot[n] = ___243d74h+0x25800*n;
		race_car_shape[n] = 0;
	}
}

// after the track and the car images are loaded: race___42824h() decodes
// the images and race___4af3ch() mirrors them for reversed tracks, both
// before this
void race_walls_build(void){

	int 		x, y, w, h, ww;
	__BYTE__ * 	mas, * row;

	// the cars' shapes, where the loader has none baked
	for(x = 0; x < 4; x++){

		if(race_car_shape[x]) continue;
		if(!car_shapes) car_shapes = malloc(4*sizeof(race_car_shapes_t));
		race_car_shapes(race_car_slot[x], &car_shapes[x]);
		race_car_shape[x] = &car_shapes[x];
	}

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
