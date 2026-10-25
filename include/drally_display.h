#ifndef __DRALLY_DISPLAY_H
#define __DRALLY_DISPLAY_H

#include "types.h"


#define W_WIDTH 	1024//800//640
#define W_HEIGHT 	768//600//480

enum { VGA3, VGA13, VESA101 };
enum { W_SHRINK, W_LETTERBOX};


void dRally_Display_init(int mode);
void dRally_Display_clean(void);

// bss.c: menu palettes as loaded from MENU.BPA, 3 bytes (r, g, b) per color
extern __POINTER__ ___1a0fb8h;	// copper.pal
extern __POINTER__ ___1a10c8h;	// bgcop.pal

#endif // __DRALLY_DISPLAY
