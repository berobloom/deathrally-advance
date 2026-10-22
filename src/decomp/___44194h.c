#include "drally.h"
#include "drally_net.h"

	extern __BYTE__ ___1a1168h[];
	extern __BYTE__ NUM_OF_CARS[];


void ___44194h(void){

	__DWORD__ 		eax, ebx, ecx, edx, edi, esi, ebp;
	__BYTE__ 		esp[0x10];
	__POINTER__ 	vp[4];


	memset(&___24cef0h[0], 0, sizeof(IPX_Address));
	memset(&___24cef0h[1], 0, sizeof(IPX_Address));
	memset(&___24cef0h[2], 0, sizeof(IPX_Address));
	memset(&___24cef0h[3], 0, sizeof(IPX_Address));
	___24e4ach->write_p = 0;
	___24e4ach->read_p = 0;
	vp[0] = (__POINTER__)&___24cef0h[B(___1a1168h)];
	vp[1] = (__POINTER__)&___24cef0h[B(___1a1168h+1)];
	vp[2] = (__POINTER__)&___24cef0h[B(___1a1168h+2)];
	vp[3] = (__POINTER__)&___24cef0h[B(___1a1168h+3)];
	___60a84h();

	while(1){

		___60a28h(2);
		eax = ___60b60h(1, D(NUM_OF_CARS), (__POINTER__)___24e4b4h, 0xa);
		if(eax == 0xff) break;
	}

	___611c0h(0xa, vp);
	___24e4ach->write_p = 0;
	___24e4ach->read_p = 0;
	___24cee0h[0]->write_p = 0;
	___24cee0h[0]->read_p = 0;
	___24cee0h[1]->write_p = 0;
	___24cee0h[1]->read_p = 0;
	___24cee0h[2]->write_p = 0;
	___24cee0h[2]->read_p = 0;
	___24cee0h[3]->write_p = 0;
	___24cee0h[3]->read_p = 0;
	return;
}
