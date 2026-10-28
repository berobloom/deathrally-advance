#include "drally.h"
#include "drally_race.h"

	extern __BYTE__ ___196dach[];

void race___4a6b4h(void);
void race___4adach(void);
void race___4af3ch(void);

// the track's buildings, which are only drawn, and on a reversed track
// everything mirrored
static void files_scene(void){

	race___4a6b4h();
	race___4adach();
	if(D(___196dach) != 0) race___4af3ch();
}

// the desktop's: from the game's files
const race_loaders_t race_loaders_files = { race___49bd4h, race_alloc, race___42824h, files_scene };
