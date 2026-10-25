#include "drally.h"
#include "drally_keyboard.h"
#include "drally_race.h"
#include "drenums.h"
#include "drmath.h"
#include "sfx.h"
#include "drally_structs_free.h"
#include "drally_net.h"
#include "race_replay.h"

#if defined(DR_MULTIPLAYER)
	extern __DWORD__ ___19bd60h;
#endif // DR_MULTIPLAYER

	extern __DWORD__ MY_CAR_IDX;
	extern int NUM_OF_CARS;
	extern int TRX_WIDTH;
	extern __POINTER__ TRX_IMA;
	extern __POINTER__ BACKBUFFER;
	extern __BYTE__ ___243d08h[];
	extern __BYTE__ ___243d0ch[];
	extern __BYTE__ ___1de580h[];
	extern __BYTE__ ___24387ch[];
	extern __BYTE__ ___243880h[];
	extern __BYTE__ ___243318h[];
	extern __BYTE__ ___243cd8h[];
	extern __BYTE__ ___243878h[];
	extern void_cb ___2432c8h;
	extern char ___19bd64h[16];
	extern __DWORD__ ___24cc58h_msx_volume;
	extern __DWORD__ ___24cc54h_sfx_volume;
	extern __BYTE__ ___196dach[];
	extern __BYTE__ ___243ca0h[];
	extern __BYTE__ ___243cdch[];
	extern __BYTE__ ___243cb8h[];
	extern __BYTE__ ___243cd4h[];
	extern __BYTE__ SUPERGLOBAL___243cd0h[];
	extern __BYTE__ ___243ce0h[];
	extern __BYTE__ ___1e6ed0h[];
	extern __BYTE__ ___196df0h[];
	extern __BYTE__ ___243334h[];
	extern __BYTE__ ___243c60h[];
	extern __BYTE__ ___2438cch[];
	extern __BYTE__ ___2438d4h[];
	extern __BYTE__ ___196ddch[];
	extern __BYTE__ ___196de0h[];
	extern __BYTE__ ___196df8h[];
	extern __BYTE__ ___2438c8h[];
	extern __BYTE__ kmap[];
	extern __BYTE__ ___243d00h[];
	extern int TRX_VIEWPORT_TL_X;
	extern int TRX_VIEWPORT_TL_Y;
	extern int CURRENT_VIEWPORT_W;
	extern int CURRENT_VIEWPORT_X;
	extern __BYTE__ ___196dd4h[];
	extern __BYTE__ ___243d14h[];
	extern __BYTE__ * VGA13_ACTIVESCREEN;
	extern int CURRENT_VIEWPORT_CENTER_X;
	extern __BYTE__ ___1de7d0h[];
	extern __BYTE__ ___196dd8h[];
	extern __DWORD__ ___243d44h;

#if defined(DR_MULTIPLAYER)
void race___44304h(void);
void race___44a78h(void);
void race___47ed8h(__DWORD__);
#endif // DR_MULTIPLAYER

void race___400ech(void);
void race___46a10h(void);
void race___45d3ch(void);
void race___560d8h(void);
void race___454ach(void);
void __VGA13_PRESENTSCREEN__(void);
__DWORD__ __GET_FRAME_COUNTER(void);
void race___4207ch(void);
void race___40f48h(void);
void race___40db4h(void);
void race___42218h(void);
void race___56594h(void);
void race___51eb4h(void);
void race___50ba4h(void);
void race___53464h(void);
void race___53310h(void);
void race___51204h(void);
void race___50a48h(void);
void race___4f170h(void);
void race___51ce0h(void);
void race___4f300h(void);
void race___50ef4h(void);
void race___52d7ch(void);
void race___4f030h(void);
void race___4ee9ch(void);
void race___46738h__VGA13_PRESENTCLEARSCREEN(void);
void race___478c8h(__DWORD__);
void race___48458h(void);
void race___49390h(void);
void race___4c21ch(void);
void race___56170h(void);
void race___55ae0h(void);
void race___54668h(void);
int rand_watcom106(void);
void race___4ff50h(void);
void race___526ach(void);
void race___514d8h(void);
void race___4dcach(void);
void race___4c434h(void);
void race___4b62ch(int);
void race___5209ch(void);
void race___563ach(void);
void race___405bch(void);
void race___496b0h(void);
void race___4af3ch(void);
void race___45a24h(void);
void race___3f970h(void);
void race___49a34h(void);
void race___40164h(void);
void ___60705h(void_cb);
void race___49bd4h(void);
void race___492f4h(void);
void dRally_Sound_stop(void);
void dRally_Sound_release(void);
void ___606dfh(void);
void ___605deh_cdecl(__DWORD__, __DWORD__);
void dRally_Sound_load(__DWORD__ msx_t, const char * msx_f, __DWORD__ sfx_t, const char * sfx_f, __DWORD__ num_ch);
void dRally_Sound_setMasterVolume(__DWORD__ vol);
void dRally_Sound_setMusicVolume(__DWORD__ vol);
void dRally_Sound_setEffectsVolume(__DWORD__ vol);
void dRally_Sound_setSampleRate(__DWORD__ freq);
void dRally_Sound_play(void);
void dRally_Sound_pushEffect(__BYTE__ channel, __BYTE__ n, __DWORD__ unk, __DWORD__ a0, __DWORD__ a1, __DWORD__ a2);
__BYTE__ dRally_Sound_setPosition(__DWORD__ pos_n);
void race___52ac4h(void);
void dRally_Sound_freeEffectChannel(__BYTE__ ch_num);
void dRally_Sound_adjustEffect(__BYTE__ channel, __DWORD__ vol, __DWORD__ freq, __DWORD__ balance);
__DWORD__ dRally_Sound_getPosition(void);
void ___60719h(void);
void srand_watcom106(__DWORD__);
void race___4539ch(void);
void race___42824h(void);
void race___4a25ch(void);
void race___4a6b4h(void);
void race___4083ch(void);
void race___4adach(void);
void race___4580ch(void);
void __VGA13_SETMODE(void);
void race_logic(void);


static __WORD__ helper22(const char * cp){

	if(!strcmp(cp, "TR0")) return 0x1e00;
	if(!strcmp(cp, "TR1")) return 0x3700;
	if(!strcmp(cp, "TR2")) return 0x2d00;
	if(!strcmp(cp, "TR3")) return 0x3200;
	if(!strcmp(cp, "TR4")) return 0x2d00;
	if(!strcmp(cp, "TR5")) return 0x3700;
	if(!strcmp(cp, "TR6")) return 0x3200;
	if(!strcmp(cp, "TR7")) return 0x3200;
	if(!strcmp(cp, "TR8")) return 0x3200;
	if(!strcmp(cp, "TR9")) return 0x3200;

	return 0;
}

static void helper_freeEffectChannels(void){
	
	int 	n;

	n = -1;
	while(++n < 0xe) dRally_Sound_freeEffectChannel(n+1);
}

static __WORD__ helper33(void){

	__DWORD__ 	ecx, ecxx;

	helper_freeEffectChannels();
	ecx = dRally_Sound_getPosition()&0xff00;
	if((ecxx = helper22(___19bd64h)) != 0) dRally_Sound_setPosition(ecxx);
	dRally_Sound_setMasterVolume(0x8000);

	return ecx;
}


// ___56774h
void race_main(int MyIndex, int NumCars){		// my_position_index, number_of_racers

	__DWORD__ 	eax, ebx, ecx, edx;
	__BYTE__ 	esp[0x1c];
	int 	i, j, n, bool_tmp, k, m, c, frames;
	struct_35e_t *	s_35e;
	struct_94_t * 	s_94;
	struct_54_t *	s_54;



	s_54 = (struct_54_t *)___1de7d0h;
	s_94 = (struct_94_t *)___1de580h;
	s_35e = (struct_35e_t *)___1e6ed0h;
	MY_CAR_IDX = MyIndex;
	NUM_OF_CARS = NumCars;
	race___3f970h();
	race___49a34h();

#if defined(DR_MULTIPLAYER)
	if(___19bd60h != 0){
			
		D(___243d08h) = 0;
		D(___243d0ch) = 0;

		n = -1;
		while(++n < NUM_OF_CARS){

			D(___243d0ch) += s_94[n].__38;
			D(___243d0ch) += s_94[n].__3c;
			D(___243d0ch) += s_94[n].__40;
			D(___243d08h) += 1;
		}

		srand_watcom106(D(___243d0ch));
		D(___24387ch) = 0;
		D(___243880h) = 1;
		D(___243318h) = 0;
		D(___243cd8h) = 0;
		D(___243878h) = 0;
	}
#endif // DR_MULTIPLAYER

	___60705h(&race___40164h);
	race___49bd4h();
	race_alloc();
	race___4539ch();
	___2432c8h = &race___492f4h;
	race___42824h();
	race___4a25ch();
	race___405bch();
	race___4a6b4h();
	race___4083ch();
	race___4adach();
	race___4580ch();
	dRally_Sound_stop();
	dRally_Sound_release();
	___606dfh();
	__VGA13_SETMODE();

	bool_tmp = 1;
#if defined(DR_MULTIPLAYER)
	if(___19bd60h != 0) bool_tmp = 0;
#endif // DR_MULTIPLAYER

	___605deh_cdecl(0x46, bool_tmp);
	dRally_Sound_load(1, strcat(strcpy(esp, ___19bd64h), "-MUS.CMF"), 2, "GEN-EFE.CMF", 0xe);
	dRally_Sound_setMasterVolume(0);
	dRally_Sound_setMusicVolume(___24cc58h_msx_volume);
	dRally_Sound_setEffectsVolume(___24cc54h_sfx_volume);
	dRally_Sound_setSampleRate(0x5622);
	dRally_Sound_play();
	dRally_Sound_setMasterVolume(0);
	if(!dRally_Race_getSettings(RACE_MSX)) dRally_Sound_setMusicVolume(0);
	if(!dRally_Race_getSettings(RACE_SFX)) dRally_Sound_setEffectsVolume(0);
	race___45a24h();
	if(D(___196dach) != 0) race___4af3ch();
	race___496b0h();
	race___405bch();
	race___49a34h();
	dRally_Sound_pushEffect(1, (s_94[MY_CAR_IDX].__0&0xff)+25, 0, 0x10000, 0x28000, 0x8000);
	race_walls_build();
	resetCounter(1);
	D(___243ca0h) = 0;
	D(___243cdch) = 0;
	D(___243cb8h) = 0;
	D(___243cd4h) = 0;
	resetCounter(5);
	resetCounter(2);
	D(SUPERGLOBAL___243cd0h) = 0;

	while(1){

#if defined(DR_TICKLOG)
		race_ticklog_before();
#endif // DR_TICKLOG
		race_logic();
#if defined(DR_TICKLOG)
		race_ticklog_after();
#endif // DR_TICKLOG

		if(kmap[DR_SCAN_F12] != 0) race___49390h();
		if(kmap[DR_SCAN_F1] != 0){

#if defined(DR_MULTIPLAYER)
			if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
				ecx = helper33();
				race___48458h();
				dRally_Sound_setPosition(ecx);
				dRally_Sound_setMasterVolume(0x10000);
				dRally_Sound_pushEffect(1, (s_94[MY_CAR_IDX].__0&0xff)+25, 0, 0x10000, 0x28000, 0x8000);
#if defined(DR_MULTIPLAYER)
			}
#endif // DR_MULTIPLAYER
		}

		if(!D(___243d00h)&&kmap[DR_SCAN_TAB]){
			
			D(___243d00h) = 1;
			dRally_Race_toggleSettings(RACE_NOTWIDE);
		}

		if(kmap[DR_SCAN_TAB] == 0) D(___243d00h) = 0;

		if((int)s_94[MY_CAR_IDX].__18 <= 0) dRally_Race_setSettings(RACE_NOTWIDE);

		if(kmap[DR_SCAN_F2] != 0){	// MSX

			dRally_Sound_setMusicVolume(dRally_Race_getSettings(RACE_MSX) ? 0 : ___24cc58h_msx_volume);
			dRally_Race_toggleSettings(RACE_MSX);
			kmap[DR_SCAN_F2] = 0;
		}

		if(kmap[DR_SCAN_F3] != 0){	// SFX

			dRally_Sound_setEffectsVolume(dRally_Race_getSettings(RACE_SFX) ? 0 : ___24cc54h_sfx_volume);
			dRally_Race_toggleSettings(RACE_SFX);
			kmap[DR_SCAN_F3] = 0;
		}

		if(kmap[DR_SCAN_F4] != 0){	// TEXTURES

			dRally_Race_toggleSettings(RACE_TEXTURES);
			kmap[DR_SCAN_F4] = 0;
		}

		if(kmap[DR_SCAN_F5] != 0){	// SHADOWS

			dRally_Race_toggleSettings(RACE_SHADOWS);
			kmap[DR_SCAN_F5] = 0;
		}

		if(kmap[DR_SCAN_P] != 0){	// GAME PAUSED
#if defined(DR_MULTIPLAYER)
			if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
				ecx = helper33();
				race_msg(E_GAME_PAUSED);
				race___478c8h(0);
				dRally_Sound_setPosition(ecx);
				dRally_Sound_setMasterVolume(0x10000);
				dRally_Sound_pushEffect(1, (s_94[MY_CAR_IDX].__0&0xff)+25, 0, 0x10000, 0x28000, 0x8000);
#if defined(DR_MULTIPLAYER)
			}
#endif // DR_MULTIPLAYER
		}

		frames = __GET_FRAME_COUNTER();

#if defined(DR_TICKLOG)
		if(getenv("DR_CHECK_NODRAW")) race_check_nodraw();
		else
#endif // DR_TICKLOG
		race_render();

		if((int)D(___2438c8h) > 0x12c){
#if defined(DR_MULTIPLAYER)
			if(___19bd60h != 0) D(___24387ch) = 0;
#endif // DR_MULTIPLAYER
			helper_freeEffectChannels();
			dRally_Sound_pushEffect(2, SFX_RACE_OVER, 0, 0x10000, 0x50000, 0x8000);
			race_msg(E_RACE_OVER);
			race___478c8h(0);
			D(___243d14h) = 1;
		}

		/*
		D(___243d08h) = __GET_FRAME_COUNTER();
		while(D(___243d08h) == __GET_FRAME_COUNTER());
		*/
		while (frames == __GET_FRAME_COUNTER());

		if((int)s_35e[MY_CAR_IDX].Drug > 0){

			race___454ach();
			__VGA13_PRESENTSCREEN__();
		}
		else {

			c = -1;
			while(++c < 0xc8) memcpy(VGA13_ACTIVESCREEN+0x140*c, BACKBUFFER+0x200*c+0x60, 0x140);

			__VGA13_PRESENTSCREEN__();
			s_35e[MY_CAR_IDX].Drug = 0;
		}

		race_render_after();

#if defined(DR_MULTIPLAYER)
		if(	(!___19bd60h&&kmap[DR_SCAN_ESCAPE])
		||	(___19bd60h&&kmap[DR_SCAN_ESCAPE]&&((int)getCounter(1) > 0xc8))
		||	(___19bd60h&&D(___243cd8h)&&((int)getCounter(1) > 0xc8))
		){
#else
		if(kmap[DR_SCAN_ESCAPE]){
#endif // DR_MULTPLAYER

#if defined(DR_MULTIPLAYER)
			if(___19bd60h != 0){

				D(___24387ch) = 0;
				if(D(___243cd8h) == 0) ___61418h(0xfd);
			}
#endif // DR_MULTIPLAYER

			D(___243d14h) = -1;
			helper_freeEffectChannels();

#if defined(DR_MULTIPLAYER)
			if((___19bd60h != 0)&&(D(___243cd8h) != 0)){

				race_msg(E_ABOUT_TO_ABORT);
				race___47ed8h(1);
			}
			else {
#endif // DR_MULTIPLAYER
				race_msg(E_ABORT_RACE);
#if defined(DR_MULTIPLAYER)
			}

			if((___19bd60h != 0)&&(D(___243cd8h) == 0)) race___47ed8h(0);
			if(___19bd60h == 0){
#endif // DR_MULTIPLAYER
				race___478c8h(0);
#if defined(DR_MULTIPLAYER)
			}
#endif // DR_MULTIPLAYER

			if(D(___243d14h) != 1){

				D(___243d14h) = 0;
				dRally_Sound_pushEffect(1, (s_94[MY_CAR_IDX].__0&0xff)+25, 0, 0x10000, 0x28000, 0x8000);
			}
			else {
#if defined(DR_MULTIPLAYER)
				if(___19bd60h == 0){
#endif // DR_MULTIPLAYER		
					race___560d8h();
#if defined(DR_MULTIPLAYER)
				}
#endif // DR_MULTIPLAYER
			}

#if defined(DR_MULTIPLAYER)
			if((___19bd60h != 0)&&(D(___243d14h) == 0)){
				
				D(___24387ch) = 1;
				D(___243cd8h) = 0;
			}
#endif // DR_MULTIPLAYER
		}









#if defined(DR_MULTIPLAYER)
		if((___19bd60h != 0)&&(D(___243318h) == 1)){

			D(___24387ch) = 0;
			___61418h(0xfc);
			D(___243d14h) = -1;
			race_msg(E_CONNECTION_LOST);
			race___47ed8h(0);
			D(___243d14h) = 1;
			s_54[0].__44 = -1;
		}
#endif // DR_MULTIPLAYER

		D(___196dd8h)++;
		if(D(___196dd8h) == 1){

			race___45d3ch();	// ENTERING RACE SCREEN ANIMATION

#if defined(DR_MULTIPLAYER)
			if(___19bd60h == 0){
#endif // DR_MULTIPLAYER

				if(___243d44h != 0){

					race_msg(E_WELCOME);
					race___478c8h(0);
				}
#if defined(DR_MULTIPLAYER)
			}
#endif // DR_MULTIPLAYER
		}

		if(D(___243d14h) != 0) break;
	} // w

#if defined(DR_TICKLOG)
	race_ticklog_end();
#endif // DR_TICKLOG
	race___46a10h();	// LEAVING RACE SCREEN ANIMATION
	race___46738h__VGA13_PRESENTCLEARSCREEN();

	n = -1;
	while(++n < NUM_OF_CARS) s_94[n].__54 = s_35e[n].Position;

	___60719h();
	dRally_Sound_stop();
	dRally_Sound_release();
	race___400ech();
	race_free();
	kmap[DR_SCAN_ESCAPE] = 0;
}
