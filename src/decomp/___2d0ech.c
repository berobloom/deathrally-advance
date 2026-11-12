#include "drally.h"
#include "drally_shop.h"
#include "drally_fonts.h"


void ___13248h_cdecl(__DWORD__, __DWORD__ ,__DWORD__, __DWORD__, __DWORD__);


// MARKET STUFF BOUGHT INFO BOX MESSAGE
void ___2d0ech_cdecl(int n){

	char40 * lines;

	___13248h_cdecl(0x90, 0x72, 0x180, 0x77, 1);

	lines = (char40 *)___193db0h[n];
	VESA101_16X16_FORMAT_PRINT(lines[0], 170, 124);
	VESA101_16X16_FORMAT_PRINT(lines[1], 170, 140);
	VESA101_16X16_FORMAT_PRINT(lines[2], 170, 156);
	VESA101_16X16_FORMAT_PRINT(lines[3], 170, 172);
	VESA101_16X16_FORMAT_PRINT(lines[4], 170, 188);
	VESA101_16X16_FORMAT_PRINT(lines[5], 170, 204);
}
