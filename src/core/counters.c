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

#if defined(DR_ALIGNED_CHECK)
// see D() in drally.h
unsigned dr_misaligned, dr_misaligned_line;
const char * dr_misaligned_file;

#if defined(__unix__)
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static void dr_misaligned_report(void){

	if(dr_misaligned) fprintf(stderr, "DR_ALIGNED_CHECK: %u unaligned accesses, the first at %s:%u\n", dr_misaligned, dr_misaligned_file, dr_misaligned_line);
	else fprintf(stderr, "DR_ALIGNED_CHECK: every access aligned\n");
}

// and when killed, as the test scripts stop the game
static void dr_misaligned_term(int sig){

	exit(1);
}

__attribute__((constructor)) static void dr_misaligned_init(void){

	atexit(dr_misaligned_report);
	signal(SIGTERM, dr_misaligned_term);
}
#endif // __unix__
#endif // DR_ALIGNED_CHECK
