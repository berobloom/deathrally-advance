#include "drally.h"
#include "drally_career.h"
#include "drally_fonts.h"
#include "drally_structs_fixed.h"

	extern __POINTER__ ___1a112ch__VESA101_ACTIVESCREEN_PTR;
	extern __POINTER__ ___1a1e80h;
	extern __BYTE__ ___196aa4h[];
	extern __BYTE__ ___1a1ef8h[];
	extern __BYTE__ ___1a01e0h[];

__DWORD__ ___14c50h(void);
void ___13248h_cdecl(__DWORD__, __DWORD__ ,__DWORD__, __DWORD__, __DWORD__);


char * itoa_watcom106(int value, char * buffer, int radix);

// STEROIDS RESULT
int ___15808h(void){

	__BYTE__ 	px;
	char 		buff[0x34], pay[12];
	int 		i, j, n, rslt;
	rslt = 0;	// ___14c50h()'s, always
	___13248h_cdecl(0x21, 0x83, 0x1e2, 0xe6, 1);

	j = -1;
	while(++j < 0x80){

		i = -1;
		while(++i < 0x68){

			if((px = B(___1a1e80h+0x68*j+i)) != 0) B(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x1a42d+0x280*j+i) = px;
		}
	}

	if((int)D(___196aa4h) <= 0){

		VESA101_16X16_FORMAT_PRINT("[Where're my steroids?", 161, 168);
		VESA101_16X16_FORMAT_PRINT("", 161, 184);
		VESA101_16X16_FORMAT_PRINT("[The whole deal went sour{ because", 161, 200);
		VESA101_16X16_FORMAT_PRINT("of your incompetence. A few slick", 161, 216);
		VESA101_16X16_FORMAT_PRINT("turns of the wheel, but no, nope!", 161, 232);
		VESA101_16X16_FORMAT_PRINT("I'm not happy, and when I'm not", 161, 248);
		VESA101_16X16_FORMAT_PRINT("happy I am mad, and when I am", 161, 264);
		VESA101_16X16_FORMAT_PRINT("mad [bad things happen - to you!", 161, 280);
		VESA101_16X16_FORMAT_PRINT("", 161, 296);
		VESA101_16X16_FORMAT_PRINT("", 161, 312);

		// a fine, or worse, see career.c
		career_event_apply(CAREER_EVENT_STEROIDS);
	}
	else {

		itoa_watcom106(career_event_apply(CAREER_EVENT_STEROIDS), pay, 10);
		VESA101_16X16_FORMAT_PRINT("[What motorized steroid peddling!", 161, 168);
		VESA101_16X16_FORMAT_PRINT("", 161, 184);
		VESA101_16X16_FORMAT_PRINT("Wow, my main man. I didn't know", 161, 200);
		VESA101_16X16_FORMAT_PRINT("there was that much smoke in the", 161, 216);
		VESA101_16X16_FORMAT_PRINT("whole burning hell, and through", 161, 232);
		VESA101_16X16_FORMAT_PRINT("it all, bearing gifts like some", 161, 248);
		VESA101_16X16_FORMAT_PRINT("blessed beast, you came. And [this", 161, 264);
		VESA101_16X16_FORMAT_PRINT(strcat(strcat(strcpy(buff, "[here is prime stuff! $"), pay), ", to you."), 161, 280);
		VESA101_16X16_FORMAT_PRINT("", 161, 296);
		VESA101_16X16_FORMAT_PRINT("", 161, 312);
	}

	___12e78h_v3(___1a10cch___185ba9h, "CONTINUE", 192, 316);

	return rslt;
}
