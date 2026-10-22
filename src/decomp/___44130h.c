#include "drally.h"
#include "drally_net.h"

	extern __BYTE__ NUM_OF_CARS[];
	extern __BYTE__ ___243878h[];
	extern __BYTE__ ___243330h[];


void ___44130h_cdecl(__DWORD__ A0){

	__DWORD__	eax, ebx, ecx, edx, edi, esi, ebp;

	eax = A0;
	ebx = eax;
	___60a84h();
___44143h:
	eax = 2;
	___60a28h(eax);
	edx = D(NUM_OF_CARS);
	eax = ebx;
	eax = ___60b48h(eax, edx);
	if(L(eax) != 0xff) goto ___44143h;
	edx = 0;
	eax = 0;
	___611c0h(eax, NULL);
	___61278h();
	resetCounter(5);
	resetCounter(2);
	resetCounter(3);
	D(___243878h) = 0;
	eax = getCounter(3);
	D(___243330h) = eax; 
	return;
}
