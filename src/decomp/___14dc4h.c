#include "drally.h"
#include "drally_career.h"
#include "drally_shop.h"
#include "drally_fonts.h"
#include "drally_structs_fixed.h"


	extern __BYTE__ ___1a01e0h[];
	extern __POINTER__ ___1a112ch__VESA101_ACTIVESCREEN_PTR;
	extern __POINTER__ ___1a1e74h;
	extern __BYTE__ ___1a1ef8h[];
	extern __POINTER__ ___1a1e84h;
	extern __POINTER__ ___1a1e60h;





void ___13248h_cdecl(__DWORD__, __DWORD__ ,__DWORD__, __DWORD__, __DWORD__);

static void helper00(const event_text_t * e, __POINTER__ pic, int ev){

	int 		i, j;
	__BYTE__ 	px;
	racer_t * 	rc;

	rc = &((racer_t *)___1a01e0h)[D(___1a1ef8h)];
	___13248h_cdecl(33, 131, 482, 230, 1);

	j = -1;
	while(++j < 128){

		i = -1;
		while(++i < 104){

			if((px = B(pic+104*j+i)) != 0) B(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x280*(168+j)+(45+i)) = px;
		}
	}

	career_event_apply(ev);	// e->rewards[rc->car], see career.c

	i = -1;
	while(++i < 10) VESA101_16X16_FORMAT_PRINT((e->spcf_i == i)?e->spcf[rc->car]:e->common[i], 161, 136+16*i);

	___12e78h_v3(___1a10cch___185ba9h, "CONTINUE", 192, 316);
}

// WINNING STREAK
void ___14dc4h(void){ helper00(&event_streak, ___1a1e74h, CAREER_EVENT_STREAK); }

// REAPER
void ___1549ch(void){ helper00(&event_reaper, ___1a1e60h, CAREER_EVENT_REAPER); }

// INCONCEIVABLE
void ___15130h(void){ helper00(&event_inconceivable, ___1a1e84h, CAREER_EVENT_INCONCEIVABLE); }
