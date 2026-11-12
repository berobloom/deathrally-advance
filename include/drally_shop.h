#ifndef __DRALLY_SHOP_H
#define __DRALLY_SHOP_H

#include "drally_structs_free.h"

// the shop's and the underground market's texts, see src/shop/shop_texts.c:
// each 6 lines of 40
extern const char40 ___191100h[4][6];	// engine installed
extern const char40 ___191880h[4][6];	// tires bought
extern const char40 ___192000h[4][6];	// armor bought
extern const char40 ___1914c0h[4][6];	// tires on show
extern const char40 ___191c40h[4][6];	// armor on show
extern const char40 MaxUpgrades[3][6];	// no more engine, tire or armor upgrades
extern const char40 ___194170h[12][6];	// repair, by the damage
extern const char40 ___193720h[4][6];	// market items on show
extern const char40 ___193db0h[4][6];	// market items bought
extern const char40 ___1923c0h[5][6];	// loan offered, by 5 less your car
extern const char40 ___192870h[5][6];	// loan granted, the same way

#endif // __DRALLY_SHOP_H
