#include "drally.h"
#include "drally_fonts.h"
#include "drally_structs_fixed.h"
#include "sfx.h"
#include "drally_career.h"

	extern __BYTE__ ___1a1ef8h[];
	extern __BYTE__ ___1a1ef0h[];
	extern __BYTE__ ___1a01e0h[];
	extern __DWORD__ ___24cc54h_sfx_volume;
	extern __BYTE__ ___185a38h_delay[];
	extern __BYTE__ ___1a0a50h[];
	extern __DWORD__ ___1a0170h_PriceMines;
	extern __DWORD__ ___1a0174h_PriceSpikedBumpers;
	extern __DWORD__ ___1a0178h_PriceRocketFuel;
	extern __DWORD__ ___1a017ch_PriceSabotage;
#if defined(DR_MULTIPLAYER)
	extern __DWORD__ ___19bd60h;
#endif // DR_MULTIPLAYER
	extern __BYTE__ ___196adch[];
	extern __BYTE__ ___185a20h[];

void ___31008h(void);
void ___3266ch(void);
void ___33010h_cdecl(int NumCars);
void ___13248h_cdecl(__DWORD__, __DWORD__ ,__DWORD__, __DWORD__, __DWORD__);
void ___2db88h(void);
void ___2da10h(void);
void ___2d898h(void);
void ___2d728h(void);
void ___2d0ech_cdecl(__DWORD__);
__DWORD__ ___28ab4h_cdecl(__DWORD__);
void ___2d6a0h(void);
void ___2d618h(void);
void ___25330h(void);
void ___12cb8h__VESA101_PRESENTSCREEN(void);
void ___26fach(void);
void dRally_Sound_pushEffect(__BYTE__ channel, __BYTE__ n, __DWORD__ unk, __DWORD__ a0, __DWORD__ a1, __DWORD__ a2);

void underground___2e350h(void){

	__DWORD__ 	eax, ebx, ecx, edx, edi, esi, ebp;
	__BYTE__ 	esp[0xc];
	int 		n;
	racer_t *	s_6c;


	s_6c = (racer_t *)___1a01e0h;
	
	switch(D(___1a1ef0h)){
	case 0: // LOANSHARK
		if((s_6c[D(___1a1ef8h)].loanshark_counter == -1)&&(career_loan_amount() != 0)){

			dRally_Sound_pushEffect(1, SFX_BUMMER, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
			career_loan_take();
			___26fach();
			D(___185a38h_delay) = 0x136;
			___25330h();
			___12cb8h__VESA101_PRESENTSCREEN();
		}
		else {

			if((s_6c[D(___1a1ef8h)].loanshark_counter == -1)&&(career_loan_amount() == 0)){

				dRally_Sound_pushEffect(2, SFX_LAUGHTER, 0, ___24cc54h_sfx_volume, 0x25500, 0x8000);
				D(___185a38h_delay) = 0x136;
				___2d618h();
				___12cb8h__VESA101_PRESENTSCREEN();
			}

			if(s_6c[D(___1a1ef8h)].loanshark_counter != -1){

				if(career_loan_owed() > (int)s_6c[D(___1a1ef8h)].money){

					dRally_Sound_pushEffect(2, SFX_LAUGHTER, 0, ___24cc54h_sfx_volume, 0x25500, 0x8000);
				}
				else {

					dRally_Sound_pushEffect(1, SFX_BUMMER, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
					career_loan_pay();
					D(___185a38h_delay) = 0x136;
					___2d6a0h();
					___25330h();
					___12cb8h__VESA101_PRESENTSCREEN();
				}
			}
		}
		break;
	case 1: // MINES
		if((career_market_state(CAREER_MARKET_MINES) == 1)&&(___28ab4h_cdecl(career_market_price(CAREER_MARKET_MINES)) == 0)){

			dRally_Sound_pushEffect(1, SFX_CLICK_4, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
			career_market_buy(CAREER_MARKET_MINES);
			___2d728h();
			___2d0ech_cdecl(0);
			D(___185a38h_delay) = 0x136;
			___25330h();
			___12cb8h__VESA101_PRESENTSCREEN();
		}
		else {

			dRally_Sound_pushEffect(2, SFX_LAUGHTER, 0, ___24cc54h_sfx_volume, 0x25500, 0x8000);
		}
		break;
	case 2: // SPIKED BUMPERS
		if((career_market_state(CAREER_MARKET_SPIKES) == 1)&&(___28ab4h_cdecl(career_market_price(CAREER_MARKET_SPIKES)) == 0)){

			dRally_Sound_pushEffect(1, SFX_CLICK_4, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
			career_market_buy(CAREER_MARKET_SPIKES);
			___2d898h();
			___2d0ech_cdecl(1);
			D(___185a38h_delay) = 0x136;
			___25330h();
			___12cb8h__VESA101_PRESENTSCREEN();
		}
		else {

			dRally_Sound_pushEffect(2, SFX_LAUGHTER, 0, ___24cc54h_sfx_volume, 0x25500, 0x8000);
		}
		break;
	case 3: // ROCKET FUEL
		if((career_market_state(CAREER_MARKET_FUEL) == 1)&&(___28ab4h_cdecl(career_market_price(CAREER_MARKET_FUEL)) == 0)){

			dRally_Sound_pushEffect(1, SFX_CLICK_4, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
			career_market_buy(CAREER_MARKET_FUEL);
			___2da10h();
			___2d0ech_cdecl(2);
			D(___185a38h_delay) = 0x136;
			___25330h();
			___12cb8h__VESA101_PRESENTSCREEN();
		}
		else {

			dRally_Sound_pushEffect(2, SFX_LAUGHTER, 0, ___24cc54h_sfx_volume, 0x25500, 0x8000);
		}
		break;
	case 4: // SABOTAGE
		if((career_market_state(CAREER_MARKET_SABOTAGE) == 1)&&(___28ab4h_cdecl(career_market_price(CAREER_MARKET_SABOTAGE)) == 0)){

			dRally_Sound_pushEffect(1, SFX_CLICK_4, 0, ___24cc54h_sfx_volume, 0x28000, 0x8000);
			career_market_buy(CAREER_MARKET_SABOTAGE);
			___2db88h();
			___2d0ech_cdecl(3);
			D(___185a38h_delay) = 0x136;
			___25330h();
			___12cb8h__VESA101_PRESENTSCREEN();
		}
		else {

			dRally_Sound_pushEffect(2, SFX_LAUGHTER, 0, ___24cc54h_sfx_volume, 0x25500, 0x8000);
		}
		break;
	case 5: // CONTINUE
		if(!career_market_can_race()){

			___13248h_cdecl(144, 114, 384, 119, 1);
			VESA101_16X16_FORMAT_PRINT("[Repair your car first.",             170, 124);
			VESA101_16X16_FORMAT_PRINT("",                                    170, 140);
			VESA101_16X16_FORMAT_PRINT("What'cha gonna do with that pile of", 170, 156);
			VESA101_16X16_FORMAT_PRINT("junk, carry it around? Let me spell", 170, 172);
			VESA101_16X16_FORMAT_PRINT("this to you: R-E-P-A-I-R.",           170, 188);
			___12cb8h__VESA101_PRESENTSCREEN();
			dRally_Sound_pushEffect(2, SFX_LAUGHTER, 0, ___24cc54h_sfx_volume, 0x25500, 0x8000);
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

			D(___185a20h) = 1;
			D(___1a1ef0h) = 4;
		}
		break;
	default:
		break;
	}
}
