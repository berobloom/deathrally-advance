#include "drally.h"
#include "drally_fonts.h"
#include "drally_structs_fixed.h"
#include "drally_career.h"

#if defined(DR_MULTIPLAYER)
	extern __DWORD__ ___19bd60h;
#endif // DR_MULTIPLAYER
	extern __BYTE__ ___1a01e0h[];

void ___12cb8h__VESA101_PRESENTSCREEN(void);
void ___17324h(void);
__DWORD__ __GET_TIMER_TICKS(void);
char * itoa_watcom106(int value, char * buffer, int radix);
void ___13248h_cdecl(__DWORD__, __DWORD__ ,__DWORD__, __DWORD__, __DWORD__);

// SABOTAGE: after the sign-up, the others' damage repaired and, if you
// bought sabotage, a rival's done (career_signup_sabotage()), and that
// told. 1 if it was
__DWORD__ ___32230h(void){

	__BYTE__	esp[0x84];
	int 		x, y, victim;
	racer_t * 	s_6c;


	s_6c = (racer_t *)___1a01e0h;
	D(esp+0x80) = 0x2d;
#if defined(DR_MULTIPLAYER)
	if(___19bd60h != 0) return 0;
#endif // DR_MULTIPLAYER
	victim = career_signup_sabotage(__GET_TIMER_TICKS());
	if(victim < 0) return 0;

	itoa_watcom106(s_6c[victim].damage, esp+0x6c, 0xa);
	strcpy(esp+0x5c, s_6c[victim].name);
	___13248h_cdecl(D(esp+0x80), 0xa5, 0x1ca, 0xc3, 1);
	x = 0x280*(0xa5+0x28)+D(esp+0x80)+0x38;
	y = x/0x280;
	x = x%0x280;
	VESA101_16X16_FORMAT_PRINT("Money talks, and [the damage is", x, y);
	strcpy(esp, "[done, ");
	strcat(esp, esp+0x6c);
	strcat(esp, "% worth! Sabotage says that");
	x = 0x280*(0xa5+0x38)+D(esp+0x80)+0x38;
	y = x/0x280;
	x = x%0x280;
	VESA101_16X16_FORMAT_PRINT(esp, x, y);
	strcpy(esp, "[");
	strcat(esp, esp+0x5c);
	strcat(esp, " is going down{ and staying");
	x = 0x280*(0xa5+0x48)+D(esp+0x80)+0x38;
	y = x/0x280;
	x = x%0x280;
	VESA101_16X16_FORMAT_PRINT(esp, x, y);
	x = 0x280*(0xa5+0x58)+D(esp+0x80)+0x38;
	y = x/0x280;
	x = x%0x280;
	VESA101_16X16_FORMAT_PRINT("down. That's dough baked luck for", x, y);
	x = 0x280*(0xa5+0x68)+D(esp+0x80)+0x38;
	y = x/0x280;
	x = x%0x280;
	VESA101_16X16_FORMAT_PRINT("you. And luck, she ain't no lady.", x, y);
	x = 0x280*(0xa5+0x73)+D(esp+0x80)+0x38;
	y = x/0x280;
	x = x%0x280;
	VESA101_16X16_FORMAT_PRINT("", x, y);
	___12e78h_v3(___1a10cch___185ba9h, "CONTINUE", 192, 315);
	___12cb8h__VESA101_PRESENTSCREEN();
	___17324h();
	return 1;
}
