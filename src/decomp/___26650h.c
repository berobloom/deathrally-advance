#include "drally.h"
#include "drally_fonts.h"
#include "drally_structs_fixed.h"
#include "drally_structs_free.h"
#include "watcom106.h"
#include "assets.h"
#include "drally_shop.h"

	extern __POINTER__ ___1a112ch__VESA101_ACTIVESCREEN_PTR;
	extern __BYTE__ ___1a1ef8h[];
	extern __BYTE__ ___1a01e0h[];
	extern cardata_t ___18e298h[6];
	extern __DWORD__ ___1862bch[][0x18];
	extern __DWORD__ ___1864fch[][0x10];
	extern __DWORD__ ___18643ch[][0xc];
	extern __POINTER__ ___1a01d0h[4];
	extern __POINTER__ ___1a0198h[4];
	extern __POINTER__ ___1a01a8h[4];
	extern __BYTE__ ___1a1eech[];
	extern __BYTE__ ___1a1ec0h[];
	extern __BYTE__ ___1a1ec8h[];

int ___25180h_cdecl(const char * A1);
void ___12f60h_cdecl(const char * A1, __DWORD__ A2);
void ___13248h_cdecl(__DWORD__, __DWORD__ ,__DWORD__, __DWORD__, __DWORD__);
void ___259e0h_cdecl(int dx, int dy, int aFrameIdx, __POINTER__ aEncoded, int * aOffsets);

enum { UPGRADE_ENGINE = 0, UPGRADE_TIRE = 1, UPGRADE_ARMOR = 2 };

static void helper_aa(int dst_offx, int dst_offy, int type){

	int 		i, j;
	__BYTE__ *	bases4;

	bases4 = getAssets(MENU_BASES4);

	j = -1;
	while(++j < 0x60){

		i = -1;
		while(++i < 0x60){

			B(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x280*(dst_offy+j)+(dst_offx+i)) = B(bases4+0x2400*type+0x60*j+i);
		}
	}
}

static void helper_yy(int idx, int dst_offx, int dst_offy, int type){

	int 		i, j;
	__BYTE__ * 	maxi1f;

	maxi1f = getAssets(MENU_MAXI1F);

	j = -1;
	while(++j < 0x40){

		i = -1;
		while(++i < 0x60){

			B(___1a112ch__VESA101_ACTIVESCREEN_PTR+0x280*(dst_offy+j)+(dst_offx+i)) = B(maxi1f+0x6000*type+0x1800*idx+0x60*j+i);
		}
	}
}

static void helper_xx(int price, int dst_x, int dst_y){

	__BYTE__ 	esp[0x28];

	itoa_watcom106(price, esp, 0xa);
	strcat(strcpy(esp+0x14, "$"), esp);
	___12f60h_cdecl(esp+0x14, ___25180h_cdecl(esp+0x14)+0x280*dst_y+dst_x);
}

static void helper_all(int n, int x1, int type, int lvl, int frame_n, __POINTER__ p, int * ip, int price, const char40 * info){

	int 	i, j;

	helper_aa(x1, 253, type);

	if(n != lvl){

		___259e0h_cdecl(x1, 269, frame_n, p, ip);
		helper_xx(price, x1, 335);
	}
	else {

		helper_yy(n, x1, 269, type);
	}

	___13248h_cdecl(144, 114, 384, 119, 1);
	VESA101_16X16_FORMAT_PRINT(info[0], 170, 124);
	VESA101_16X16_FORMAT_PRINT(info[1], 170, 140);
	VESA101_16X16_FORMAT_PRINT(info[2], 170, 156);
	VESA101_16X16_FORMAT_PRINT(info[3], 170, 172);
	VESA101_16X16_FORMAT_PRINT(info[4], 170, 188);
	VESA101_16X16_FORMAT_PRINT(info[5], 170, 204);
}

// SHOP ENGINES
void ___25e40h(void){

	int 			n;
	racer_t * 		s_6c;
	const char40 * 	info;
	cardata_t * 	cdp;

	s_6c = (racer_t *)___1a01e0h;
	cdp = (cardata_t *)&___18e298h[s_6c[D(___1a1ef8h)].car];
	n = s_6c[D(___1a1ef8h)].engine;
	info = (n < cdp->n_engine_upgrades )?cdp->txt_engine_upgrades[n]:MaxUpgrades[UPGRADE_ENGINE];

	helper_all(n-1, 16, UPGRADE_ENGINE, cdp->n_engine_upgrades-1, D(___1a1ec8h), ___1a01a8h[n], ___1862bch[n], cdp->price_engine_upgrades[n], info);
}

// SHOP ENGINE UPGRADE INSTALLED
void ___273d8h(void){

	int 			n;
	racer_t * 		s_6c;
	const char40 * 	info;
	cardata_t * 	cdp;

	s_6c = (racer_t *)___1a01e0h;
	cdp = (cardata_t *)&___18e298h[s_6c[D(___1a1ef8h)].car];
	n = s_6c[D(___1a1ef8h)].engine;
	info = ___191100h[n];

	helper_all(n, 16, UPGRADE_ENGINE, cdp->n_engine_upgrades-1, D(___1a1ec8h), ___1a01a8h[n], ___1862bch[n], cdp->price_engine_upgrades[n+1], info);
}

// SHOP TIRES
void ___262b4h(void){

	int 			n;
	racer_t * 		s_6c;
	const char40 * 	info;
	cardata_t * 	cdp;

	s_6c = (racer_t *)___1a01e0h;
	cdp = (cardata_t *)&___18e298h[s_6c[D(___1a1ef8h)].car];
	n = s_6c[D(___1a1ef8h)].tires;
	info = (n < cdp->n_tire_upgrades )?___1914c0h[n]:MaxUpgrades[UPGRADE_TIRE];

	helper_all(n-1, 120, UPGRADE_TIRE, cdp->n_tire_upgrades-1, D(___1a1ec0h), ___1a0198h[n], ___18643ch[n], cdp->price_tire_upgrades[n], info);
}

// SHOP TIRES UPGRADE INSTALLED
void ___276f0h(void){

	int 			n;
	racer_t * 		s_6c;
	const char40 * 	info;
	cardata_t * 	cdp;

	s_6c = (racer_t *)___1a01e0h;
	cdp = (cardata_t *)&___18e298h[s_6c[D(___1a1ef8h)].car];
	n = s_6c[D(___1a1ef8h)].tires;
	info = ___191880h[n];

	helper_all(n, 120, UPGRADE_TIRE, cdp->n_tire_upgrades-1, D(___1a1ec0h), ___1a0198h[n], ___18643ch[n], cdp->price_tire_upgrades[n+1], info);
}

// SHOP ARMOR
void ___26650h(void){

	int 			n;
	racer_t * 		s_6c;
	const char40 * 	info;
	cardata_t * 	cdp;

	s_6c = (racer_t *)___1a01e0h;
	cdp = (cardata_t *)&___18e298h[s_6c[D(___1a1ef8h)].car];
	n = s_6c[D(___1a1ef8h)].armor;
	info = (n < cdp->n_armor_upgrades)?___191c40h[n]:MaxUpgrades[UPGRADE_ARMOR];

	helper_all(n-1, 224, UPGRADE_ARMOR, cdp->n_armor_upgrades-1, D(___1a1eech), ___1a01d0h[n], ___1864fch[n], cdp->price_armor_upgrades[n], info);
}

// SHOP ARMOR UPGRADE INSTALLED
void ___27a10h(void){

	int 			n;
	racer_t * 		s_6c;
	const char40 * 	info;
	cardata_t * 	cdp;

	s_6c = (racer_t *)___1a01e0h;
	cdp = (cardata_t *)&___18e298h[s_6c[D(___1a1ef8h)].car];
	n = s_6c[D(___1a1ef8h)].armor;
	info = ___192000h[n];

	helper_all(n, 224, UPGRADE_ARMOR, cdp->n_armor_upgrades-1, D(___1a1eech), ___1a01d0h[n], ___1864fch[n], cdp->price_armor_upgrades[n+1], info);
}
