#include "drally.h"
#include "drally_race.h"

	extern int TRX_WIDTH;
	extern int TRX_HEIGHT;
	extern __POINTER__ TRX_MAS;

// Which 8x8 blocks of the track hold a wall: a mask value below 4. The
// race code tests the 40x40 pixels under a car against the mask every
// tick, so it asks here first and skips the test where no wall is near,
// which finds nothing the test wouldn't.

#define WALL_BLOCK	3

static __BYTE__ * 	walls;
static int 		walls_w, walls_h;

void race_walls_build(void){

	int 	x, y;

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
int race_walls_near(int x, int y, int w, int h){

	int 	bx, by;

	if((x < 0)||(y < 0)||(x+w > TRX_WIDTH)||(y+h > TRX_HEIGHT)) return 1;

	for(by = y >> WALL_BLOCK; by <= (y+h-1) >> WALL_BLOCK; by++){
		for(bx = x >> WALL_BLOCK; bx <= (x+w-1) >> WALL_BLOCK; bx++){

			if(walls[walls_w*by+bx]) return 1;
		}
	}

	return 0;
}
