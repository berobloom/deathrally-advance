// The race's sound on the GBA, through Maxmod: the track's music (its S3M)
// and the effects the race code asks for with dRally_Sound_pushEffect()
// and the rest, as the desktop's sound_api.c takes them. tools/gbabake
// bakes both; the Makefile puts them in a soundbank with mmutil.

#include <stdlib.h>
#include <gba_base.h>
#include <gba_interrupt.h>
#include <maxmod.h>

#include "drally.h"
#include "sound.h"

extern const u8 soundbank_bin[];
extern const u8 sfx_bin[];		// each effect's base volume, 0-64
extern const u8 men_bin[];		// and each of the menus'

// the effects' ids come first in the soundbank, then a module for each track
#define SFX_FIRST		0
#define SFX_CHANNELS		14	// the race's effect channels, 1-14
#define MOD_CHANNELS		16	// the most an S3M of the game's has
#define MIX_CHANNELS		(MOD_CHANNELS+8)

// mmFrame() runs in the vblank interrupt, and Maxmod isn't reentrant: a
// vblank in the middle of mmStart() left mmReadPattern() looping on a
// half set up song. So the race's calls hold the interrupts off
#define LOCKED(...)		do { u16 ime = REG_IME; REG_IME = 0; __VA_ARGS__; REG_IME = ime; } while(0)

static mm_sfxhand 	handles[SFX_CHANNELS+1];
static u8 		effect[SFX_CHANNELS+1];	// what each plays, 1 on

// Maxmod's rate is 6.10, the game's 16.16; its volume 0-255 for the
// game's 16.16 times the sample's own 0-64; its panning 0-255, the
// game's balance 0-0x10000
static mm_word rate(__DWORD__ freq){ freq >>= 6; return (freq > 0xffff) ? 0xffff : freq; }
static mm_byte pan(__DWORD__ balance){ balance >>= 8; return (balance > 255) ? 255 : balance; }
static mm_word volume(int n, __DWORD__ vol){

	__DWORD__ 	v = ((vol >> 8)*sfx_bin[n-1]) >> 6;

	return (v > 255) ? 255 : v;
}

void sound_init(void){

	static mm_gba_system 	mm;

	mm.mixing_mode = MM_MIX_13KHZ;
	mm.mod_channel_count = MOD_CHANNELS;
	mm.mix_channel_count = MIX_CHANNELS;
	// all in EWRAM: IWRAM, where the stack is, has none to spare
	mm.module_channels = (mm_addr)malloc(MOD_CHANNELS*MM_SIZEOF_MODCH);
	mm.active_channels = (mm_addr)malloc(MIX_CHANNELS*MM_SIZEOF_ACTCH);
	mm.mixing_channels = (mm_addr)malloc(MIX_CHANNELS*MM_SIZEOF_MIXCH);
	mm.mixing_memory = (mm_addr)malloc(MM_MIXLEN_13KHZ);
	mm.wave_memory = (mm_addr)malloc(MM_MIXLEN_13KHZ);
	mm.soundbank = (mm_addr)soundbank_bin;
	mmInit(&mm);
	mmSetModuleVolume(768);
	mmSetEffectsVolume(1024);
}

static int 	menu_playing;

void sound_music(int n){

	LOCKED(mmEffectCancelAll(); if(n < 0) mmStop(); else mmStart(n, MM_PLAY_LOOP));
	menu_playing = 0;
}

// the menus' music (MEN-MUS.CMF) from its pattern order, as the desktop's
// dRally_Sound_setPosition(order << 8) jumps in it, or going on where it
// is if order < 0. Started if it isn't playing
void sound_menu_music(int order){

	if(!menu_playing){

		LOCKED(mmStart(MOD_MEN, MM_PLAY_LOOP); mmPosition((order < 0) ? 0x2d : order));
		menu_playing = 1;
	}
	else if(order >= 0) LOCKED(mmPosition(order));
}

// where the menus' music is, for sound_menu_music() to go back to
int sound_menu_position(void){

	return menu_playing ? (int)mmGetPosition() : -1;
}

// sfx_channel 1-14 plays effect n (GEN-EFE.CMF's instrument n) at vol and
// freq times its own, balance 0x8000 in the middle. Maxmod can't start
// partway through, so offset is left out
void dRally_Sound_pushEffect(__BYTE__ sfx_channel, __BYTE__ n, __DWORD__ offset, __DWORD__ vol, __DWORD__ freq, __DWORD__ balance){

	mm_sound_effect 	e;

	if(!sfx_channel||(sfx_channel > SFX_CHANNELS)||!n) return;
	e.id = SFX_FIRST+n-1;
	e.rate = rate(freq);
	e.handle = 0;
	e.volume = volume(n, vol);
	e.panning = pan(balance);
	LOCKED(mmEffectCancel(handles[sfx_channel]); handles[sfx_channel] = mmEffectEx(&e));
	effect[sfx_channel] = n;
}

void dRally_Sound_adjustEffect(__BYTE__ channel, __DWORD__ vol, __DWORD__ freq, __DWORD__ balance){

	if(!channel||(channel > SFX_CHANNELS)||!effect[channel]) return;
	mm_word 	r = rate(freq), v = volume(effect[channel], vol);
	mm_byte 	b = pan(balance);

	LOCKED(mmEffectRate(handles[channel], r); mmEffectVolume(handles[channel], v); mmEffectPanning(handles[channel], b));
}

void dRally_Sound_freeEffectChannel(__BYTE__ ch_num){

	if(!ch_num||(ch_num > SFX_CHANNELS)) return;
	LOCKED(mmEffectCancel(handles[ch_num]));
	handles[ch_num] = 0;
	effect[ch_num] = 0;
}

// the menus' effect n (MEN-SAM.CMF's instrument n, sfx.h's numbers) on
// channel 1-14 at freq times its own, as the menus' calls give it, the
// one there before cut short
void sound_menu(int channel, int n, __DWORD__ freq){

	mm_sound_effect 	e;
	__DWORD__ 		v;

	if(!channel||(channel > SFX_CHANNELS)||(n < 1)||(n > SFX_MEN31-SFX_MEN01+1)) return;
	v = (0xc000 >> 8)*men_bin[n-1] >> 6;	// the menus' default volume
	e.id = SFX_MEN01+n-1;
	e.rate = rate(freq);
	e.handle = 0;
	e.volume = (v > 255) ? 255 : v;
	e.panning = 128;
	LOCKED(mmEffectCancel(handles[channel]); handles[channel] = mmEffectEx(&e));
	effect[channel] = 0;
}
