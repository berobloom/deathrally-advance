#include "drally.h"
#include "drally_net.h"

#if defined(DR_MULTIPLAYER)
	extern __DWORD__ ___19bd60h;
#endif // DR_MULTIPLAYER


void ___10754h(void){

#if defined(DR_MULTIPLAYER)
	if((___19bd60h != 0)&&(CONNECTION_TYPE == 2)) ___63b20h(1, ___199f9ch);   
#endif // DR_MULTIPLAYER
}
