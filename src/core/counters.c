#include "drally.h"

	extern __BYTE__ ___243ca4h[];
	extern __BYTE__ SUPERGLOBAL___243898h[];
	extern __BYTE__ SUPERGLOBAL___243894h[];
	extern __BYTE__ ___243ca8h[];
	extern __BYTE__ ___243874h[];
	extern __BYTE__ ___2438d0h[];

void incCounter(int n){

	if(n == 1) D(___243ca4h)++;
	if(n == 2) D(SUPERGLOBAL___243898h)++;
	if(n == 3) D(SUPERGLOBAL___243894h)++;
	if(n == 4) D(___243ca8h)++;
	if(n == 5) D(___243874h)++;
	if(n == 6) D(___2438d0h)++;
}

void decCounter(int n){

	if(n == 1) D(___243ca4h)--;
	if(n == 2) D(SUPERGLOBAL___243898h)--;
	if(n == 3) D(SUPERGLOBAL___243894h)--;
	if(n == 4) D(___243ca8h)--;
	if(n == 5) D(___243874h)--;
	if(n == 6) D(___2438d0h)--;
}

__DWORD__ getCounter(int n){

	if(n == 1) return D(___243ca4h);
	if(n == 2) return D(SUPERGLOBAL___243898h);
	if(n == 3) return D(SUPERGLOBAL___243894h);
	if(n == 4) return D(___243ca8h);
	if(n == 5) return D(___243874h);
	if(n == 6) return D(___2438d0h);

	return 0;
}

void resetCounter(int n){

	if(n == 1) D(___243ca4h) = 0;
	if(n == 2) D(SUPERGLOBAL___243898h) = 0;
	if(n == 3) D(SUPERGLOBAL___243894h) = 0;
	if(n == 4) D(___243ca8h) = 0;
	if(n == 5) D(___243874h) = 0;
	if(n == 6) D(___2438d0h) = 0;
}

void setCounter(int n, __DWORD__ val){

	if(n == 1) D(___243ca4h) = val;
	if(n == 2) D(SUPERGLOBAL___243898h) = val;
	if(n == 3) D(SUPERGLOBAL___243894h) = val;
	if(n == 4) D(___243ca8h) = val;
	if(n == 5) D(___243874h) = val;
	if(n == 6) D(___2438d0h) = val;
}
