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

#define A_PI 	3.14159265359
#define L0_83 	0.833333

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

static void helper01(double d_sign){

	struct_35e_t * s_35e;
	struct_35e_t * 	car;
	__POINTER__ 	s94;

	s_35e = (struct_35e_t *)___1e6ed0h;
	// the car this is about, read once: through D() the compiler would
	// read and scale the index again for every use
	car = s_35e+D(___243c60h);
	s94 = ___1de580h+0x94*D(___243c60h);

	if((car->__18 == 0)&&(car->__1c == 0)){

		car->Direction += (float)(d_sign*(double)car->__a8);
		F32(___243cfch) += (float)((double)F32(s94+0x10)*d_sign*(double)car->__a8/90.0);
		F32(___243cfch) *= (float)(1.0+d_sign*(double)car->__bc/18.0);

		if(car->__c0*d_sign < 36.0f) car->__c0 += (float)(2.0*d_sign);
	}
}

// RACE ENGINE
void race___4c434h(void){

	double 	d_tmp, d_hypotenuse, d_a, d_b, d_sign;
	__DWORD__ 	eax, ebx, ecx, edx, edi, esi;
	__BYTE__ 	esp[0x64];
	int 	breaker, i, j;
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
	car->__b0 *= 0.98;
	
	if((int)D(s94+0x18) > 0){

		if(car->Finished == 0){

			L(ecx) = car->Ctrls[getCounter(4)];

			if((L(ecx)&CTRL_ACCELERATE) != 0){
				
				if(((L(ecx)&CTRL_TURBO_BOOST) == 0)||((int)D(s94+0x34) <= 0)){

					car->__b0 += (float)(0.8*(double)F32(s94+4)/30.0);
				}
				else {

					if(D(s94+0x20) == 0){

						car->__b0 += (float)(1.3*(double)F32(s94+4)/30.0);
					}
					else {

						car->__b0 += (float)(1.5*(double)F32(s94+4)/30.0);

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

				car->__b0 -= F32(s94+4)/90.0;
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

			car->__bc *= (float)(9.0/10.0);
		}
		else {

			if(0.0 >= (double)F32(s94+4)){

				car->__bc *= (float)(9.0/10.0);
			}
			else {

				car->__bc = (float)((((double)car->__c0*(double)car->__b0)*(double)F32(s94+0xc))/(double)F32(s94+4));

				if(dRMath_abs((double)car->__bc) > 20.0){

					car->__b0 *= (float)(49.0/50.0);
				}
			}
		}

		if((car->Ctrls[getCounter(4)]&CTRL_STEER_RIGHT) != 0) helper01(-1.0);
		if(car->Direction < 0.0f) car->Direction += 360.0f;
		if((car->Ctrls[getCounter(4)]&CTRL_STEER_LEFT) != 0) helper01(1.0);
		if(car->Direction >= 360.0f) car->Direction -= 360.0f;

		if(car->__c0 > 0.0f) car->__c0 -= 1.0f;
		if(car->__c0 < 0.0f) car->__c0 += 1.0f;

		if((int)D(s94+0x18) > 0){
		
			car->Direction += car->__104;
		}

		if(car->Direction < 0.0f) car->Direction += 360.0f;
		if(car->Direction >= 360.0f) car->Direction -= 360.0f;
	} // i

	F32(esp+0x38) = (float)((double)car->__bc*dRMath_sin_deg(((double)car->Direction+ 90.0))/11.0);
	F32(esp+0x3c) = (float)((double)car->__bc*dRMath_cos_deg(((double)car->Direction+ 90.0))*L0_83/11.0);
	F32(esp+0x34) = (float)((double)car->__b0*dRMath_sin_deg(((double)car->Direction+180.0)));
	F32(esp+0x30) = (float)((double)car->__b0*dRMath_cos_deg(((double)car->Direction+180.0))*L0_83);
	F32(___243d24h) = (float)((double)F32(___243cfch)*dRMath_sin_deg(((double)car->Direction+270.0)));
	F32(___243d20h) = (float)((double)F32(___243cfch)*dRMath_cos_deg(((double)car->Direction+270.0))*L0_83);

	d_a = (double)F32(esp+0x34)+(double)F32(esp+0x38);
	d_b = (double)F32(esp+0x30)+(double)F32(esp+0x3c);

	d_hypotenuse = dRMath_magnitude2(d_a, d_b);
	F32(esp+0x58) = (d_hypotenuse == 0.0)?1.0f:(float)(dRMath_abs((double)car->__b0)/d_hypotenuse);

	car->__15a = (float)((double)car->__fc+(double)F32(___243d24h)+(double)F32(esp+0x58)*d_a);
	car->__15e = (float)((double)car->__100+(double)F32(___243d20h)+(double)F32(esp+0x58)*d_b);

	car->__c4 = (float)(18.0*dRMath_sin_deg(((double)car->Direction+206.0))+(double)car->__15a);
	car->__c8 = (float)(18.0*dRMath_cos_deg(((double)car->Direction+206.0))*L0_83+(double)car->__15e);
	car->__cc = (float)(18.0*dRMath_sin_deg(((double)car->Direction+154.0))+(double)car->__15a);
	car->__d0 = (float)(18.0*dRMath_cos_deg(((double)car->Direction+154.0))*L0_83+(double)car->__15e);
	car->__d4 = (float)(18.0*dRMath_sin_deg(((double)car->Direction+334.0))+(double)car->__15a);
	car->__d8 = (float)(18.0*dRMath_cos_deg(((double)car->Direction+334.0))*L0_83+(double)car->__15e);
	car->__dc = (float)(18.0*dRMath_sin_deg(((double)car->Direction+ 26.0))+(double)car->__15a);
	car->__e0 = (float)(18.0*dRMath_cos_deg(((double)car->Direction+ 26.0))*L0_83+(double)car->__15e);
	car->__e4 = (float)( 8.0*dRMath_sin_deg(((double)car->Direction+270.0))+(double)car->__15a);
	car->__e8 = (float)( 8.0*dRMath_cos_deg(((double)car->Direction+270.0))*L0_83+(double)car->__15e);
	car->__ec = (float)( 8.0*dRMath_sin_deg(((double)car->Direction+ 90.0))+(double)car->__15a);
	car->__f0 = (float)( 8.0*dRMath_cos_deg(((double)car->Direction+ 90.0))*L0_83+(double)car->__15e);
	car->__f4 = (float)(22.0*dRMath_sin_deg(((double)car->Direction+180.0))+(double)car->__15a);
	car->__f8 = (float)(22.0*dRMath_cos_deg(((double)car->Direction+180.0))*L0_83+(double)car->__15e);

	D(___243d24h) = 0;
	D(___243d20h) = 0;

	edx = (int)((double)car->YLocation+(double)car->__f8);
	D(esp+0x5c) = (int)((double)car->XLocation+(double)car->__f4);
	D(esp+0x2c) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)((double)car->YLocation+(double)car->__c8);
	D(esp+0x5c) = (int)((double)car->XLocation+(double)car->__c4);
	D(esp+0x50) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)((double)car->YLocation+(double)car->__d0);
	D(esp+0x5c) = (int)((double)car->XLocation+(double)car->__cc);
	D(esp+0x48) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)((double)car->YLocation+(double)car->__d8);
	D(esp+0x5c) = (int)((double)car->XLocation+(double)car->__d4);
	D(esp+0x4c) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)((double)car->YLocation+(double)car->__e0);
	D(esp+0x5c) = (int)((double)car->XLocation+(double)car->__dc);
	D(esp+0x54) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)((double)car->YLocation+(double)car->__e8);
	D(esp+0x5c) = (int)((double)car->XLocation+(double)car->__e4);
	D(esp+0x40) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	edx = (int)((double)car->YLocation+(double)car->__f0);
	D(esp+0x5c) = (int)((double)car->XLocation+(double)car->__ec);
	D(esp+0x44) = helper00(edx*TRX_WIDTH+D(esp+0x5c));

	car->ImgIndex = (int)((double)car->Direction/3.75);
	car->ImgOffset = 0x25800*D(___243c60h)+0x640*car->ImgIndex;
	ecx = (int)((double)car->__15e+(double)car->YLocation-20.0);
	ebxp = ___243d74h+(int)car->ImgOffset;
	edip = TRX_MAS+(int)(double)car->XLocation-0x14+TRX_WIDTH*(int)ecx+(int)(double)car->__15a;
	esi = 0xf;

	// the car's pixels against walls under them, unless there are none
	j = race_walls_near((int)car->XLocation-0x14+(int)car->__15a, ecx, 0x28, 0x28) ? -1 : 0x28;
	while(++j < 0x28){

		breaker = 0;

		i = -1;
		while(++i < 0x28){

			if(((signed char)B(ebxp+0x28*j+i) > 3)&&((signed char)(B(edip+TRX_WIDTH*j+i)&0xf) < 4)){

				esi = B(edip+TRX_WIDTH*j+i)&0xf;
				D(___243cb0h) = i;
				D(___243cb4h) = j;
				breaker = 1;
				break;
			}
		}

		if(breaker == 1) break;
	}

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
				car->__162 = (float)(double)(int)D(esp+0x5c);
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
				D(esp+0x5c) = (int)((double)car->YLocation+(double)car->__15e-20.0);
				edi = TRX_WIDTH*D(esp+0x5c);
				car->XLocation += car->__162;
				edip = TRX_MAS+(int)car->XLocation+(int)edi-0x14+(int)(double)car->__15a;
				ebxp = ___243d74h+car->ImgOffset;
				esi = 0xf;
				edx = 0;
				eax = 0;

				ecx = 0;
				if(race_walls_near((int)car->XLocation-0x14+(int)car->__15a, D(esp+0x5c), 0x28, 0x28)) while(1){

					if((signed char)B(ebxp) <= 3){

						edip++;
						ebxp++;
						ecx++;

						if(ecx == 0x28){

							edip = edip+TRX_WIDTH-0x28;
							edx++;
							if(edx == 0x28) break;
							ecx = 0;
						}
					}
					else {

						L(eax) = B(edip)&0xf;

						if((signed char)L(eax) >= 4){

							edip++;
							ebxp++;
							ecx++;

							if(ecx == 0x28){

								edip = edip+TRX_WIDTH-0x28;
								edx++;
								if(edx == 0x28) break;
								ecx = 0;
							}
						}
						else {

							esi = eax;
							D(___243cb0h) = ecx;
							D(___243cb4h) = edx;
							break;
						}
					}
				} // w

				car->__14 = esi;

				if((int)esi < 4){
					
					car->YLocation -= car->__166;
					car->XLocation -= car->__162;
				}
			} // w
		} // i

		if((car->__192 != 0)&&((int)car->__192 <= 3)){

			car->__fc = (float)(-1.0*(double)car->__15a);
			car->__100 = (float)(-1.0*(double)car->__15e);
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

			car->__b0 *= (float)(7.0/8.0);

			car->__14 = 1;
		} // i
	} // i

	if((D(esp+0x50) == 4)||(D(esp+0x48) == 4)||(D(esp+0x4c) == 4)||(D(esp+0x54) == 4)||(D(esp+0x40) == 4)||(D(esp+0x44) == 4)){

		car->__b0 *= (float)(34.0/35.0);
	}

	if((D(esp+0x48) == 5)||(D(esp+0x4c) == 5)){

		car->__104 += 0.2f;
		car->__b0 *= (float)(29.0/30.0);
	}

	if((D(esp+0x50) == 5)||(D(esp+0x54) == 5)){

		car->__104 -= 0.2f;
		car->__b0 *= (float)(29.0/30.0);
	}

	if((D(esp+0x50) == 6)||(D(esp+0x48) == 6)||(D(esp+0x4c) == 6)||(D(esp+0x54) == 6)||(D(esp+0x40) == 6)||(D(esp+0x44) == 6)){

		car->__b0 *= (float)(8.0/9.0);
	}

	if((D(esp+0x50) == 7)||(D(esp+0x48) == 7)||(D(esp+0x4c) == 7)||(D(esp+0x54) == 7)||(D(esp+0x40) == 7)||(D(esp+0x44) == 7)){

		car->__b0 *= (float)(19.0/20.0);
	}

	if((D(esp+0x50) == 0xa)||(D(esp+0x4c) == 0xa)){

		car->__104 -= 0.15f;
	}

	if((D(esp+0x48) == 0xa)||(D(esp+0x54) == 0xa)){

		car->__104 += 0.15f;
	}

	if((D(esp+0x50) == 0xb)||(D(esp+0x48) == 0xb)||(D(esp+0x4c) == 0xb)||(D(esp+0x54) == 0xb)||(D(esp+0x40) == 0xb)||(D(esp+0x44) == 0xb)){

		car->__b0 *= (float)(76.0/75.0);
	}

	if((D(esp+0x50) == 0xc)||(D(esp+0x4c) == 0xc)){

		car->__104 -= 0.05f;
	}

	if((D(esp+0x48) == 0xc)||(D(esp+0x54) == 0xc)){

		car->__104 += 0.05f;
	}

	if((D(esp+0x50) == 0xd)||(D(esp+0x48) == 0xd)||(D(esp+0x4c) == 0xd)||(D(esp+0x54) == 0xd)||(D(esp+0x40) == 0xd)||(D(esp+0x44) == 0xd)){

		F64(esp+0x10) = 7.0*(double)car->__b0;
		car->Direction += (float)((double)rand_watcom106()*F64(esp+0x10)*(1.0/65536.0));
		F64(esp+0x10) = 7.0*(double)car->__b0;
		car->Direction -= (float)((double)rand_watcom106()*F64(esp+0x10)*(1.0/65536.0));
		if(car->Direction < 0.0f) car->Direction += 360.0f;
		if(car->Direction >= 360.0f) car->Direction -= 360.0f;
		car->__b0 *= (float)(29.0/30.0);
	}

	if((D(esp+0x50) == 0xe)||(D(esp+0x48) == 0xe)||(D(esp+0x4c) == 0xe)||(D(esp+0x54) == 0xe)||(D(esp+0x40) == 0xe)||(D(esp+0x44) == 0xe)){

		car->__b0 *= (float)(39.0/40.0);
	}

	car->XLocation += car->__15a;
	car->YLocation += car->__15e;

	F32(esp+0x28) = (float)(int)(TRX_WIDTH-0x14);
	if(car->XLocation > F32(esp+0x28)) car->XLocation = F32(esp+0x28);
	if(car->XLocation < 20.0) car->XLocation = 20.0f;

	F32(esp+0x24) = (float)(int)(TRX_HEIGHT-0x14);		
	if(car->YLocation > F32(esp+0x24)) car->YLocation = F32(esp+0x24);
	if(car->YLocation < 20.0) car->YLocation = 20.0f;
}
