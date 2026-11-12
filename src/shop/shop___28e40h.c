#include "drally.h"
#include "drally_fonts.h"
#include "drally_keyboard.h"
#include "drally_structs_fixed.h"
#include "drally_structs_free.h"
#include "sfx.h"
#include "drally_display.h"
#include "drally_career.h"

	extern __BYTE__ ___1a1ef8h[];
	extern __BYTE__ ___1a01e0h[];
	extern __BYTE__ ___185a14h_UseWeapons[];
	extern __BYTE__ ___1a1ec4h[];
	extern __BYTE__ ___1a1ee4h[];
	extern __DWORD__ ___24cc54h_sfx_volume;
	extern cardata_t ___18e298h[6];
#define CARENCS ___185cbch
	extern __DWORD__ ___185cbch[][0x40];
	extern __BYTE__ ___1a1ed0h[];
	extern __POINTER__ ___1a01b8h[6];
	extern __POINTER__ ___1a112ch__VESA101_ACTIVESCREEN_PTR;
	extern __BYTE__ ___1a1e68h[];
	extern __POINTER__ ___1a10f4h;
#if defined(DR_MULTIPLAYER)
	extern __DWORD__ ___19bd60h;
#endif // DR_MULTIPLAYER
	extern __POINTER__ ___1a1e88h;
	extern __BYTE__ ___196a84h[];
	extern __POINTER__ ___1a1eb4h;
	extern __BYTE__ ___185a38h_delay[];
	extern __BYTE__ ___1a1028h[];
	extern __BYTE__ ___185a4ch[];
	extern __BYTE__ ___196adch[];
	extern __BYTE__ ___185a20h[];

char * itoa_watcom106(int value, char * buffer, int radix);
void ___31008h(void);
void ___3266ch(void);
void ___33010h_cdecl(int NumCars);
void underground_main(void);
__DWORD__ underground___28c1ch(void);
void ___27a10h(void);
void ___276f0h(void);
void ___273d8h(void);
void ___27078h(void);
void ___11378h_cdecl_float(float A1, float A2, float A3);
void ___25330h(void);
void ___25e40h(void);
void ___26650h(void);
void ___262b4h(void);
void ___269e4h(void);
void ___25a74h(void);
__BYTE__ ___5994ch(void);
#if defined(DR_MULTIPLAYER)
void ___23758h(void);
#endif // DR_MULTIPLAYER
void ___12cb8h__VESA101_PRESENTSCREEN(void);
void ___2ab50h(void);
void ___259e0h_cdecl(int dx, int dy, int aFrameIdx, __POINTER__ aEncoded, int * aOffsets);
void dRally_Sound_pushEffect(__BYTE__ channel, __BYTE__ n, __DWORD__ unk, __DWORD__ a0, __DWORD__ a1, __DWORD__ a2);
void ___13248h_cdecl(__DWORD__, __DWORD__ ,__DWORD__, __DWORD__, __DWORD__);
__DWORD__ ___28ab4h_cdecl(__DWORD__);
void ___1398ch__VESA101_PRESENTRECTANGLE(__DWORD__ offset, __POINTER__ src, __DWORD__ w, __DWORD__ h);

// SHOP ENTER
void shop___28e40h(void){

	__DWORD__ 	eax, ecx, edx, esi, ebp;
	__BYTE__ 	__esp[0xc+0x60];
	__BYTE__ * 	esp = __esp+0xc;
	int 			i, j, n;
	__BYTE__ 		px;
	racer_t * 		s_6c;
	cardata_t * 	cdp;

	s_6c = (racer_t *)___1a01e0h;
	cdp = (cardata_t *)___18e298h;


	// the trade-in, see career.c
	ebp = career_shop_trade_in();

	switch(D(___1a1ec4h)){
	case 0:	// BUY NEW CAR
		if(___28ab4h_cdecl(career_shop_price(CAREER_SHOP_CAR)) == 0){

			dRally_Sound_pushEffect(1, SFX_CLICK_4, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
			___13248h_cdecl(0x90, 0x72, 0x180, 0x77, 1);
			itoa_watcom106(ebp, esp+0x3c, 0xa);
			___12e78h_v3(___1a110ch___185c0bh, strcat(strcat(strcpy(esp, "You would get a $"), esp+0x3c), " refund"), 170, 124);
			___12e78h_v3(___1a110ch___185c0bh, strcpy(esp, "from your old car and upgrades."), 170, 140);

			if((int)(cdp[D(___1a1ee4h)].price-ebp) >= 0){

				itoa_watcom106(cdp[D(___1a1ee4h)].price-ebp, esp+0x3c, 0xa);
				___12e78h_v3(___1a110ch___185c0bh, strcat(strcat(strcat(strcpy(esp, cdp[D(___1a1ee4h)].name), " would cost "), "$"), esp+0x3c), 170, 156);
				___12e78h_v3(___1a110ch___185c0bh, strcpy(esp, "Purchase it ?"), 170, 172);
			}
			else {

				itoa_watcom106(ebp-cdp[D(___1a1ee4h)].price, esp+0x3c, 0xa);
				___12e78h_v3(___1a110ch___185c0bh, strcat(strcat(strcpy(esp, "Money returned: "), "$"), esp+0x3c), 170, 156);
				___12e78h_v3(___1a110ch___185c0bh, strcat(strcat(strcpy(esp, "Buy a "), cdp[D(___1a1ee4h)].name), " ?"), 170, 172);
			}

			___12e78h_v3(___1a10cch___185ba9h, "yes", 240, 185);
			___12e78h_v3(___1a10e0h___185ba9h, "no", 410, 185);
			___12cb8h__VESA101_PRESENTSCREEN();

			ebp = 0;
			D(esp+0x54) = 0;
			while(D(esp+0x54) == 0){

				___2ab50h();
				___2ab50h();
				___259e0h_cdecl(0x10, 0x8d, D(___1a1ed0h), ___1a01b8h[D(___1a1ee4h)], CARENCS[D(___1a1ee4h)]);
				___1398ch__VESA101_PRESENTRECTANGLE(0x16090, ___1a112ch__VESA101_ACTIVESCREEN_PTR+0x16090, 0x60, 0x40);
				esi = 0xaa*ebp+0x1e0d9;
				D(___1a1ed0h)++;
				
				if((int)D(___1a1ed0h) > 0x3f) D(___1a1ed0h) = 0;

				n = -1;
				while(++n < 0x14) memset(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x280*n+esi, 0xc4, 0x14);

				j = -1;
				while(++j < 0x14){

					i = -1;
					while(++i < 0x14){

						if((px = B(___1a10f4h+0x190*D(___1a1e68h)+0x14*j+i)) != 0) B(___1a112ch__VESA101_ACTIVESCREEN_PTR+esi+0x280*j+i) = px;
					}
				}

				___1398ch__VESA101_PRESENTRECTANGLE(0x1e0a5, ___1a112ch__VESA101_ACTIVESCREEN_PTR+0x1e0a5, 0x14f, 0x1c);
				D(___1a1e68h)++;
				
				if((int)D(___1a1e68h) > 0x31) D(___1a1e68h) = 0;

				switch(___5994ch()){
				case DR_SCAN_ESCAPE:
					ebp = 1;
					D(esp+0x54) = -1;
					break;
				case DR_SCAN_ENTER:
				case DR_SCAN_KP_ENTER:
					dRally_Sound_pushEffect(1, SFX_CLICK_4, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
					D(esp+0x54) = -1;
					break;
				case DR_SCAN_F1:
#if defined(DR_MULTIPLAYER)
					if(___19bd60h != 0) ___23758h();
#endif // DR_MULTIPLAYER
					break;
				case DR_SCAN_Y:
				case DR_SCAN_LEFT:
				case DR_SCAN_KP_4:
					if(ebp == 1) dRally_Sound_pushEffect(1, SFX_CLICK_2, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
					
					ebp = 0;
					n = -1;
					while(++n < 0x19) memset(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x280*n+0x1e000+0xa8, 0xc4, 0x14f);

					___12e78h_v3(___1a10cch___185ba9h, "yes", 240, 185);
					___12e78h_v3(___1a10e0h___185ba9h, "no", 410, 185);
					break;
				case DR_SCAN_N:
				case DR_SCAN_RIGHT:
				case DR_SCAN_KP_6:
					if(ebp == 0) dRally_Sound_pushEffect(1, SFX_CLICK_2, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
					
					ebp = 1;
					n = -1;
					while(++n < 0x19) memset(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x280*n+0x1e000+0xa8, 0xc4, 0x14f);

					___12e78h_v3(___1a10e0h___185ba9h, "yes", 240, 185);
					___12e78h_v3(___1a10cch___185ba9h, "no", 410, 185);
					break;
				default:
					break;
				}
			}

			if(ebp != 0){

				___25a74h();
				___12cb8h__VESA101_PRESENTSCREEN();
			}
			else {

				dRally_Sound_pushEffect(5, SFX_NEW_CAR_BOUGHT, 0, ___24cc54h_sfx_volume, 0x24000, 0x8000);
				// it sets the next car on show, which the menu sets again below
				eax = D(___1a1ee4h);
				career_shop_buy(CAREER_SHOP_CAR);
				D(___1a1ee4h) = eax;
				___25330h();
				___25e40h();
				___26650h();
				___262b4h();
				___269e4h();
				___25a74h();
				___13248h_cdecl(0x90, 0x72, 0x180, 0x77, 1);

				j = -1;
				while(++j < 0x10){

					i = -1;
					while(++i < 0x126){

						if((px = B(___1a1e88h+0x126*j+i)) != 0) B(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x1e83c+0x280*j+i) = px;
					}
				}

				___12e78h_v3(___1a110ch___185c0bh, strcpy(esp, "Use arrows to change the color"), 170, 124);
				___12e78h_v3(___1a110ch___185c0bh, strcpy(esp, "of your car and press Enter"), 170, 140);
				___12e78h_v3(___1a110ch___185c0bh, strcpy(esp, "when finished."), 170, 156);
				___12cb8h__VESA101_PRESENTSCREEN();

				B(esp+0x5c) = 0;
				while(1){

					if((B(esp+0x5c) == DR_SCAN_KP_ENTER)||(D(___196a84h) != 0)) break;
					B(esp+0x5c) = ___5994ch();

					switch(B(esp+0x5c)){
					case DR_SCAN_F1:
#if defined(DR_MULTIPLAYER)
						if(___19bd60h != 0) ___23758h();
#endif // DR_MULTIPLAYER
						break;
					case DR_SCAN_KP_4:
					case DR_SCAN_LEFT:
						if((int)s_6c[D(___1a1ef8h)].color > 0) s_6c[D(___1a1ef8h)].color -= 2;
						break;
					case DR_SCAN_KP_6:
					case DR_SCAN_RIGHT:
						if((int)s_6c[D(___1a1ef8h)].color < 0xfd) s_6c[D(___1a1ef8h)].color += 2;
						break;
					default:
						break;
					}

					___11378h_cdecl_float(
						(float)B(___1a0fb8h+3*s_6c[D(___1a1ef8h)].color+0),
						(float)B(___1a0fb8h+3*s_6c[D(___1a1ef8h)].color+1),
						(float)B(___1a0fb8h+3*s_6c[D(___1a1ef8h)].color+2)
					);

					n = -1;
					while(++n < 0x18) memset(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x1dd80+0x280*n+0xb6, 0xc4, 0x126);
					
					j = -1;
					while(++j < 0x10){

						i = -1;
						while(++i < 0x126){

							if((px = B(___1a1e88h+0x126*j+i)) != 0) B(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x1e83c+0x280*j+i) = px;
						}
					}

					j = -1;
					while(++j < 0x18){

						i = -1;
						while(++i < 0xa){

							if((px = B(___1a1eb4h+0xa*j+i)) != 0) B(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x1de3c +0x280*j+i+s_6c[D(___1a1ef8h)].color+0xe) = px;
						}
					}

					___2ab50h();
					___2ab50h();
					___1398ch__VESA101_PRESENTRECTANGLE(
						s_6c[D(___1a1ef8h)].color+0x1de48,
						___1a112ch__VESA101_ACTIVESCREEN_PTR+0x1de3c+s_6c[D(___1a1ef8h)].color+0xc,
						0xe,
						0x18);
					___259e0h_cdecl(0x10, 0x8d, D(___1a1ed0h), ___1a01b8h[D(___1a1ee4h)], CARENCS[D(___1a1ee4h)]);
					___1398ch__VESA101_PRESENTRECTANGLE(0x16090, ___1a112ch__VESA101_ACTIVESCREEN_PTR+0x16090, 0x60, 0x40);
					D(___1a1ed0h)++;

					if((int)D(___1a1ed0h) > 0x3f) D(___1a1ed0h) = 0;

					if(B(esp+0x5c) == DR_SCAN_ENTER) break;
				}

				if((int)s_6c[D(___1a1ef8h)].car >= 5){

					D(___1a1ee4h) = 5;
				}
				else {

					D(___1a1ee4h) = s_6c[D(___1a1ef8h)].car+1;
				}

				___27078h();
				D(___185a38h_delay) = 0x136;
				___12cb8h__VESA101_PRESENTSCREEN();
			}
		}
		break;
	case 1:	// BUY ENGINE
		if((int)career_shop_price(CAREER_SHOP_ENGINE) == CAREER_SHOP_NONE){

			___25e40h();
			___12cb8h__VESA101_PRESENTSCREEN();
		}
		else {

			if(___28ab4h_cdecl(career_shop_price(CAREER_SHOP_ENGINE)) == 0){
				
				dRally_Sound_pushEffect(1, SFX_CLICK_4, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
				// the installed text goes by the level before
				___273d8h();
				D(___185a38h_delay) = 0x136;
				career_shop_buy(CAREER_SHOP_ENGINE);
				___25330h();
				___12cb8h__VESA101_PRESENTSCREEN();
			}
		}
		break;
	case 2:	// BUY TIRES
		if((int)career_shop_price(CAREER_SHOP_TIRES) == CAREER_SHOP_NONE){

			___262b4h();
			___12cb8h__VESA101_PRESENTSCREEN();
		}
		else {

			if(___28ab4h_cdecl(career_shop_price(CAREER_SHOP_TIRES)) == 0){
				
				dRally_Sound_pushEffect(1, SFX_CLICK_4, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
				// the installed text goes by the level before
				___276f0h();
				D(___185a38h_delay) = 0x136;
				career_shop_buy(CAREER_SHOP_TIRES);
				___25330h();
				___12cb8h__VESA101_PRESENTSCREEN();
			}
		}
		break;
	case 3:	// BUY ARMOR
		if((int)career_shop_price(CAREER_SHOP_ARMOR) == CAREER_SHOP_NONE){

			___26650h();
			___12cb8h__VESA101_PRESENTSCREEN();
		}
		else {

			if(___28ab4h_cdecl(career_shop_price(CAREER_SHOP_ARMOR)) == 0){
				
				dRally_Sound_pushEffect(1, SFX_CLICK_4, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
				// the installed text goes by the level before
				___27a10h();
				D(___185a38h_delay) = 0x136;
				career_shop_buy(CAREER_SHOP_ARMOR);
				___25330h();
				___12cb8h__VESA101_PRESENTSCREEN();
			}
		}
		break;
	case 4:	// REPAIR
		if((int)s_6c[D(___1a1ef8h)].damage > 0){
			
			if(___28ab4h_cdecl(career_shop_price(CAREER_SHOP_REPAIR)) == 0){
				
				dRally_Sound_pushEffect(1, SFX_ANVIL, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
				career_shop_buy(CAREER_SHOP_REPAIR);
				___25330h();
				___269e4h();
				___12cb8h__VESA101_PRESENTSCREEN();
			}
		}
		break;
	case 5:	// CONTINUE
		if(!career_shop_can_race()){

			___13248h_cdecl(144, 114, 384, 119, 1);
			VESA101_16X16_FORMAT_PRINT("[Repair your car first.",             170, 124);
			VESA101_16X16_FORMAT_PRINT("",                                    170, 140);
			VESA101_16X16_FORMAT_PRINT("What'cha gonna do with that pile of", 170, 156);
			VESA101_16X16_FORMAT_PRINT("junk, carry it around? Let me spell", 170, 172);
			VESA101_16X16_FORMAT_PRINT("this out for you: R-E-P-A-I-R.",      170, 188);
			___12cb8h__VESA101_PRESENTSCREEN();
			dRally_Sound_pushEffect(2, SFX_LAUGHTER, 0, ___24cc54h_sfx_volume, 0x25500, 0x8000);
		}
		else {

			D(___185a4ch) = underground___28c1ch();
#if defined(DR_MULTIPLAYER)
			if(___19bd60h != 0) D(___1a1028h) = 0;
#endif // DR_MULTIPLAYER
			if((D(___185a14h_UseWeapons) != 0)&&(D(___185a4ch) != 0)){

				underground_main();
			}
			else {

				dRally_Sound_pushEffect(1, SFX_SIGNUP_ENGINE_ON, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
#if defined(DR_MULTIPLAYER)
				if(___19bd60h != 0){

					___33010h_cdecl(D(___196adch));
				}
				else {
#endif // DR_MULTIPLAYER
					ecx = 0;
					n = -1;
					while(++n < 0x14){

						if(((int)ecx < (int)s_6c[n].points)&&(n != D(___1a1ef8h))) ecx = s_6c[n].points;
					}

					if((int)ecx < (int)s_6c[D(___1a1ef8h)].points){

						___31008h();	// challenge adversary
					}
					else {

						___3266ch();	// sign up
					}
#if defined(DR_MULTIPLAYER)
				}
#endif // DR_MULTIPLAYER
			}
#if defined(DR_MULTIPLAYER)
			if(___19bd60h != 0) D(___185a20h) = 1;
#endif // DR_MULTIPLAYER
		}
		break;
	default:
		break;
	}
}
