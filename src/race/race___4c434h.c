#include "drally.h"
#include "drmath.h"
#include "drally_structs_free.h"
#include "drally_race.h"

	extern __BYTE__ ___243c60h[];
	extern __BYTE__ ___1e6ed0h[];
	extern __BYTE__ ___243cfch[];
	extern __BYTE__ ___1de580h[];
	extern __BYTE__ MY_CAR_IDX[];
	extern __BYTE__ ___196db8h[];
	extern __BYTE__ ___243d24h[];
	extern __BYTE__ ___243d20h[];
	extern int TRX_WIDTH;
	extern int TRX_HEIGHT;
	extern __POINTER__ TRX_MAS;
	extern __POINTER__ ___243d74h;
	extern __BYTE__ ___243cb0h[];
	extern __BYTE__ ___243cb4h[];

#define L0_83 	0.833333f

#define CTRL_NULL			0x00
#define CTRL_ACCELERATE		0x01
#define CTRL_BRAKE			0x02
#define CTRL_STEER_LEFT		0x04
#define CTRL_STEER_RIGHT	0x08
#define CTRL_TURBO_BOOST	0x10
#define CTRL_MACHINE_GUN	0x20
#define CTRL_DROP_MINE		0x40
#define CTRL_HORN			0x42

int rand_watcom106(void);

static __DWORD__ helper00(int i){

	__DWORD__	edx, rslt;

	if(i < 0){

		rslt = 0xf;
	}
	else {

		edx = TRX_WIDTH*TRX_HEIGHT;

		if(i >= (int)edx){

			rslt = 0xf;
		}
		else {

			rslt = B(TRX_MAS+i)&0xf;
		}
	}

	return rslt;
}

// the first of the car's 40x40 pixels, row by row, that is solid in its
// image (img) and a wall in the mask under it (mas, the pixel under the
// image's first): the wall's mask value, with its column and row in
// ___243cb0h and ___243cb4h, or 0xf if there is none. Only the pixels in
// the 8x8 blocks of the wall map that have walls are searched
DR_HOT static __DWORD__ helper02(__BYTE__ * img, __BYTE__ * mas, int x, int y){

	int 		i, j, k, end, i0, i1, w = TRX_WIDTH;
	unsigned 	cells;

	for(j = 0; j < 0x28; j = end){

		end = ((y+j)|7)+1-y;
		if(end > 0x28) end = 0x28;
		if(!(cells = race_walls_cells(x, y+j, 0x28, end-j))) continue;

		for(; j < end; j++){

			// the block columns, at most 6 of them, from x's
			for(k = 0; k < 6; k++){

				if(!(cells&(1u << k))) continue;

				i0 = ((x >> 3)+k)*8-x;
				i1 = i0+8;
				if(i0 < 0) i0 = 0;
				if(i1 > 0x28) i1 = 0x28;

				for(i = i0; i < i1; i++){

					if(((signed char)img[0x28*j+i] > 3)&&((mas[w*j+i]&0xf) < 4)){

						D(___243cb0h) = i;
						D(___243cb4h) = j;

						return mas[w*j+i]&0xf;
					}
				}
			}
		}
	}

	return 0xf;
}

static void helper01(float d_sign){

	struct_35e_t * s_35e;
	struct_35e_t * 	car;
	__POINTER__ 	s94;

	s_35e = (struct_35e_t *)___1e6ed0h;
	// the car this is about, read once: through D() the compiler would
	// read and scale the index again for every use
	car = s_35e+D(___243c60h);
	s94 = ___1de580h+0x94*D(___243c60h);

	if((car->__18 == 0)&&(car->__1c == 0)){

		car->Direction += d_sign*car->__a8;
		F32(___243cfch) += F32(s94+0x10)*d_sign*car->__a8*(1.0f/90.0f);
		F32(___243cfch) *= 1.0f+d_sign*car->__bc*(1.0f/18.0f);

		if(car->__c0*d_sign < 36.0f) car->__c0 += 2.0f*d_sign;
	}
}

// RACE ENGINE
void race___4c434h(void){

	float 	d_hypotenuse, d_a, d_b, sin_d, cos_d;
	int 	a_d;
	__DWORD__ 	eax, ebx, ecx, edx, edi, esi;
	__BYTE__ 	esp[0x64] __attribute__((aligned(4)));	// D() loads words here, see aligned.mk
	__POINTER__ 	ebxp;
	__POINTER__ 	edip;
	struct_35e_t * 	s_35e;	
	struct_35e_t * 	car;
	__POINTER__ 	s94;


	s_35e = (struct_35e_t *)___1e6ed0h;
	car = s_35e+D(___243c60h);
	s94 = ___1de580h+0x94*D(___243c60h);
	D(___243cfch) = 0;
	car->__15e = 0.0f;
	car->__15a = 0.0f;
	car->__b0 *= 0.98f;
	
	if((int)D(s94+0x18) > 0){

		if(car->Finished == 0){

			L(ecx) = car->Ctrls[getCounter(4)];

			if((L(ecx)&CTRL_ACCELERATE) != 0){
				
				if(((L(ecx)&CTRL_TURBO_BOOST) == 0)||((int)D(s94+0x34) <= 0)){

					car->__b0 += F32(s94+4)*(0.8f/30.0f);
				}
				else {

					if(D(s94+0x20) == 0){

						car->__b0 += F32(s94+4)*(1.3f/30.0f);
					}
					else {

						car->__b0 += F32(s94+4)*(1.5f/30.0f);

						D(s94+0x18) -= 0x16;
						if((int)D(s94+0x18) < 0) D(s94+0x18) = 0;

						D(s94+0x58) = 1;
					}
				}

				car->__b0 *= 0.986f;
			}

			if(D(MY_CAR_IDX) == D(___243c60h)){

				if(D(s94+0x58) == 1) D(___196db8h)++;
				if(D(s94+0x58) == 0) D(___196db8h) = 0;
			}

			L(edx) = car->Ctrls[getCounter(4)];

			if(((L(edx)&CTRL_BRAKE) != 0)&&((L(edx)&CTRL_DROP_MINE) == 0)){

				car->__b0 -= F32(s94+4)*(1.0f/90.0f);
				car->__b0 *= 0.986f;
			}

			if((car->Ctrls[getCounter(4)]&CTRL_TURBO_BOOST) != 0){

				if((int)D(s94+0x34) > 0){

					if((int)D(s94+0x18) > 0){

						D(s94+0x34) -= 0x190;
						
						if((int)D(s94+0x34) < 0) D(s94+0x34) = 0;
					}
				}
			}

			if((car->Ctrls[getCounter(4)]&CTRL_TURBO_BOOST) == 0){

				if((int)D(s94+0x18) > 0){

					D(s94+0x34) += 0x14;
					if((int)D(s94+0x34) > 0x19000) D(s94+0x34) = 0x19000;
				}
			}
		}

		if((int)D(s94+0x18) <= 0) D(s94+4) = 0;

		if(car->Finished != 0){

			car->__bc *= 9.0f/10.0f;
		}
		else {

			if(0.0f >= F32(s94+4)){

				car->__bc *= 9.0f/10.0f;
			}
			else {

				car->__bc = car->__c0*car->__b0*F32(s94+0xc)/F32(s94+4);

				if(fabsf(car->__bc) > 20.0f){

					car->__b0 *= 49.0f/50.0f;
				}
			}
		}

		if((car->Ctrls[getCounter(4)]&CTRL_STEER_RIGHT) != 0) helper01(-1.0f);
		if(car->Direction < 0.0f) car->Direction += 360.0f;
		if((car->Ctrls[getCounter(4)]&CTRL_STEER_LEFT) != 0) helper01(1.0f);
		if(car->Direction >= 360.0f) car->Direction -= 360.0f;

		if(car->__c0 > 0.0f) car->__c0 -= 1.0f;
		if(car->__c0 < 0.0f) car->__c0 += 1.0f;

		if((int)D(s94+0x18) > 0){
		
			car->Direction += car->__104;
		}

		if(car->Direction < 0.0f) car->Direction += 360.0f;
		if(car->Direction >= 360.0f) car->Direction -= 360.0f;
	} // i

	// sin(d+90) is cos(d), sin(d+180) -sin(d) and so on: the 20 sines and
	// cosines of the car's direction plus a fixed angle come from three
	a_d = dRMath_turns_degf(car->Direction);
	sin_d = dRMath_fixf(dRMath_sin_i(a_d), 28);
	cos_d = dRMath_fixf(dRMath_cos_i(a_d), 28);

	F32(esp+0x38) = car->__bc*cos_d*(1.0f/11.0f);
	F32(esp+0x3c) = -car->__bc*sin_d*L0_83*(1.0f/11.0f);
	F32(esp+0x34) = -car->__b0*sin_d;
	F32(esp+0x30) = -car->__b0*cos_d*L0_83;
	F32(___243d24h) = -F32(___243cfch)*cos_d;
	F32(___243d20h) = F32(___243cfch)*sin_d*L0_83;

	d_a = F32(esp+0x34)+F32(esp+0x38);
	d_b = F32(esp+0x30)+F32(esp+0x3c);

	d_hypotenuse = sqrtf(d_a*d_a+d_b*d_b);
	F32(esp+0x58) = (d_hypotenuse == 0.0f)?1.0f:fabsf(car->__b0)/d_hypotenuse;

	car->__15a = car->__fc+F32(___243d24h)+F32(esp+0x58)*d_a;
	car->__15e = car->__100+F32(___243d20h)+F32(esp+0x58)*d_b;

	// the corners at 26 and 154 degrees, 18 pixels out, and the points
	// opposite them; 8 pixels to the sides and 22 to the back
	{
		int 	s26 = dRMath_sin_i(a_d+(int)((26u << 24)/360)), c26 = dRMath_cos_i(a_d+(int)((26u << 24)/360));
		int 	s154 = dRMath_sin_i(a_d+(int)((154u << 24)/360)), c154 = dRMath_cos_i(a_d+(int)((154u << 24)/360));
		float 	x26 = dRMath_fixf(18*(s26 >> 4), 24), y26 = dRMath_fixf(18*(c26 >> 4), 24)*L0_83;
		float 	x154 = dRMath_fixf(18*(s154 >> 4), 24), y154 = dRMath_fixf(18*(c154 >> 4), 24)*L0_83;

		car->__c4 = car->__15a-x26;
		car->__c8 = car->__15e-y26;
		car->__cc = car->__15a+x154;
		car->__d0 = car->__15e+y154;
		car->__d4 = car->__15a-x154;
		car->__d8 = car->__15e-y154;
		car->__dc = car->__15a+x26;
		car->__e0 = car->__15e+y26;
		car->__e4 = car->__15a-8.0f*cos_d;
		car->__e8 = car->__15e+8.0f*sin_d*L0_83;
		car->__ec = car->__15a+8.0f*cos_d;
		car->__f0 = car->__15e-8.0f*sin_d*L0_83;
		car->__f4 = car->__15a-22.0f*sin_d;
		car->__f8 = car->__15e-22.0f*cos_d*L0_83;
	}

	D(___243d24h) = 0;
	D(___243d20h) = 0;

	edx = (int)(car->YLocation+car->__f8);
	D(esp+0x5c) = (int)(car->XLocation+car->__f4);
	D(esp+0x2c) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)(car->YLocation+car->__c8);
	D(esp+0x5c) = (int)(car->XLocation+car->__c4);
	D(esp+0x50) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)(car->YLocation+car->__d0);
	D(esp+0x5c) = (int)(car->XLocation+car->__cc);
	D(esp+0x48) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)(car->YLocation+car->__d8);
	D(esp+0x5c) = (int)(car->XLocation+car->__d4);
	D(esp+0x4c) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)(car->YLocation+car->__e0);
	D(esp+0x5c) = (int)(car->XLocation+car->__dc);
	D(esp+0x54) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)(car->YLocation+car->__e8);
	D(esp+0x5c) = (int)(car->XLocation+car->__e4);
	D(esp+0x40) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)(car->YLocation+car->__f0);
	D(esp+0x5c) = (int)(car->XLocation+car->__ec);
	D(esp+0x44) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	// 96 images, 3.75 degrees each
	car->ImgIndex = ((a_d&0xffffff)*96) >> 24;
	car->ImgOffset = 0x25800*D(___243c60h)+0x640*car->ImgIndex;
	ecx = (int)(car->__15e+car->YLocation-20.0f);
	ebxp = ___243d74h+(int)car->ImgOffset;
	edip = TRX_MAS+(int)car->XLocation-0x14+TRX_WIDTH*(int)ecx+(int)car->__15a;
	esi = helper02(ebxp, edip, (int)car->XLocation-0x14+(int)car->__15a, ecx);

	car->__14 = esi;

	if((int)esi >= 4){

		car->__14 = 0;
	}
	else {

		car->__172 = D(___243cb0h)-0x14;
		edx = D(___243cb4h);
		car->__18e = esi;
		edx = edx-0x14;
		ecx = car->__192;
		car->__176 = edx;

		if((int)ecx > 3){

			car->__b0 = 0.0f;
			car->__16a = 1;
			car->__16e = 0;

			while(1){

				if((int)car->__14 >= 4){

					car->__14 = 0;
					break;
				}

				eax = rand_watcom106();
				ebx = 2*car->__16a+1;
				edx = (int)eax>>0x1f;
				edx = (int)eax%(int)ebx;
				edx = edx-car->__16a;
				D(esp+0x5c) = edx;
				car->__162 = (float)(int)D(esp+0x5c);
				eax = rand_watcom106();
				ebx = 2*car->__16a+1;
				edx = (int)eax>>0x1f;
				edx = (int)eax%(int)ebx;
				edi = car->__16e+1;
				D(esp+0x5c) = edx-car->__16a;
				eax = edi;
				edx = (int)eax>>0x1f;
				edx = (int)eax%5;
				car->__16e = edi;
				car->__166 = (float)(int)D(esp+0x5c);

				if(edx == 0){
										
					if((int)car->__16a < 0x1e) car->__16a++; 
				}

				car->YLocation += car->__166;
				D(esp+0x5c) = (int)(car->YLocation+car->__15e-20.0f);
				edi = TRX_WIDTH*D(esp+0x5c);
				car->XLocation += car->__162;
				edip = TRX_MAS+(int)car->XLocation+(int)edi-0x14+(int)car->__15a;
				ebxp = ___243d74h+car->ImgOffset;
				esi = helper02(ebxp, edip, (int)car->XLocation-0x14+(int)car->__15a, D(esp+0x5c));

				car->__14 = esi;

				if((int)esi < 4){
					
					car->YLocation -= car->__166;
					car->XLocation -= car->__162;
				}
			} // w
		} // i

		if((car->__192 != 0)&&((int)car->__192 <= 3)){

			car->__fc = -car->__15a;
			car->__100 = -car->__15e;
			car->__b0 = 0.0f;
			car->__14 = 1;
		}

		if(car->__192 == 0){
	
			if(((int)D(esp+0x48) < 4)||((int)D(esp+0x4c) < 4)){

				if((int)D(esp+0x2c) >= 4){
					
					car->Direction += 20.0f;
				}
			}

			if(((int)D(esp+0x50) < 4)||((int)D(esp+0x54) < 4)){

				if((int)D(esp+0x2c) >= 4){
				
					car->Direction -= 20.0f;
				}
			}

			if(car->Direction < 0.0f) car->Direction += 360.0f;
			if(car->Direction >= 360.0f) car->Direction -= 360.0f;

			car->__b0 *= 7.0f/8.0f;

			car->__14 = 1;
		} // i
	} // i

	if((D(esp+0x50) == 4)||(D(esp+0x48) == 4)||(D(esp+0x4c) == 4)||(D(esp+0x54) == 4)||(D(esp+0x40) == 4)||(D(esp+0x44) == 4)){

		car->__b0 *= 34.0f/35.0f;
	}

	if((D(esp+0x48) == 5)||(D(esp+0x4c) == 5)){

		car->__104 += 0.2f;
		car->__b0 *= 29.0f/30.0f;
	}

	if((D(esp+0x50) == 5)||(D(esp+0x54) == 5)){

		car->__104 -= 0.2f;
		car->__b0 *= 29.0f/30.0f;
	}

	if((D(esp+0x50) == 6)||(D(esp+0x48) == 6)||(D(esp+0x4c) == 6)||(D(esp+0x54) == 6)||(D(esp+0x40) == 6)||(D(esp+0x44) == 6)){

		car->__b0 *= 8.0f/9.0f;
	}

	if((D(esp+0x50) == 7)||(D(esp+0x48) == 7)||(D(esp+0x4c) == 7)||(D(esp+0x54) == 7)||(D(esp+0x40) == 7)||(D(esp+0x44) == 7)){

		car->__b0 *= 19.0f/20.0f;
	}

	if((D(esp+0x50) == 0xa)||(D(esp+0x4c) == 0xa)){

		car->__104 -= 0.15f;
	}

	if((D(esp+0x48) == 0xa)||(D(esp+0x54) == 0xa)){

		car->__104 += 0.15f;
	}

	if((D(esp+0x50) == 0xb)||(D(esp+0x48) == 0xb)||(D(esp+0x4c) == 0xb)||(D(esp+0x54) == 0xb)||(D(esp+0x40) == 0xb)||(D(esp+0x44) == 0xb)){

		car->__b0 *= 76.0f/75.0f;
	}

	if((D(esp+0x50) == 0xc)||(D(esp+0x4c) == 0xc)){

		car->__104 -= 0.05f;
	}

	if((D(esp+0x48) == 0xc)||(D(esp+0x54) == 0xc)){

		car->__104 += 0.05f;
	}

	if((D(esp+0x50) == 0xd)||(D(esp+0x48) == 0xd)||(D(esp+0x4c) == 0xd)||(D(esp+0x54) == 0xd)||(D(esp+0x40) == 0xd)||(D(esp+0x44) == 0xd)){

		car->Direction += (float)rand_watcom106()*(7.0f*car->__b0)*(1.0f/65536.0f);
		car->Direction -= (float)rand_watcom106()*(7.0f*car->__b0)*(1.0f/65536.0f);
		if(car->Direction < 0.0f) car->Direction += 360.0f;
		if(car->Direction >= 360.0f) car->Direction -= 360.0f;
		car->__b0 *= 29.0f/30.0f;
	}

	if((D(esp+0x50) == 0xe)||(D(esp+0x48) == 0xe)||(D(esp+0x4c) == 0xe)||(D(esp+0x54) == 0xe)||(D(esp+0x40) == 0xe)||(D(esp+0x44) == 0xe)){

		car->__b0 *= 39.0f/40.0f;
	}

	car->XLocation += car->__15a;
	car->YLocation += car->__15e;

	F32(esp+0x28) = (float)(int)(TRX_WIDTH-0x14);
	if(car->XLocation > F32(esp+0x28)) car->XLocation = F32(esp+0x28);
	if(car->XLocation < 20.0f) car->XLocation = 20.0f;

	F32(esp+0x24) = (float)(int)(TRX_HEIGHT-0x14);		
	if(car->YLocation > F32(esp+0x24)) car->YLocation = F32(esp+0x24);
	if(car->YLocation < 20.0f) car->YLocation = 20.0f;
}
