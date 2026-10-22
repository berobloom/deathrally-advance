#include "drally.h"
#include "netpage.h"
#include "drally_net.h"

void ___60a84h(void){

	int 	n;

	n = -1;
	while(++n < 10) ___24e3f0h[n] = 0;
	___24e3f0h[0] = ___199fb8h;
	
	___199fb6h = 1;

	// original clears 5*0x400 bytes from ___24cf28h, running into ___24df28h right after it
	memset(___24cf28h, 0, sizeof(___24cf28h));
	memset(___24df28h, 0, sizeof(___24df28h));

	memset(___24e596h, 0, 0xa);
	npg_zero(___24e4c0h);
	___6168ch();
	npg_zero(___24e4ach);
}
