// gba/race's sound, see sound.c
void sound_init(void);
void sound_music(int n);		// track n's music from the start, or none if n < 0
void sound_menu(int channel, int n, __DWORD__ freq);	// a menu's effect
void dRally_Sound_pushEffect(__BYTE__ sfx_channel, __BYTE__ n, __DWORD__ offset, __DWORD__ vol, __DWORD__ freq, __DWORD__ balance);
void dRally_Sound_adjustEffect(__BYTE__ channel, __DWORD__ vol, __DWORD__ freq, __DWORD__ balance);
void dRally_Sound_freeEffectChannel(__BYTE__ ch_num);
