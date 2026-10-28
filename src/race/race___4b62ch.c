#include "drally.h"
#include "drally_race.h"
#include "drmath.h"
#include "drally_structs_free.h"

	extern __BYTE__ ___1e6ed0h[];
	extern __BYTE__ NUM_OF_CARS[];
	extern __BYTE__ ___243cfch[];
	extern __BYTE__ ___1df520h[];
	extern int TRX_WIDTH;
	extern int TRX_HEIGHT;
	extern __POINTER__ TRX_LR1;
	extern __BYTE__ ___1de580h[];
	extern __POINTER__ TRX_VAI;
	extern int TRX_WIDTH_QTR;
	extern int TRX_OHI_DAT[0x100];
	extern float TRX_DRV_DAT[0x200];


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

static int helper00(int u, int v){ return ((u<v)?((u<0)?0:u):((v<0)?0:v))>>2; }

static void helper_drug(int eax, int edx, int i, int action, int idx){

	int 	ecx;
	struct_35e_t * 	s_35e;
	struct_94_t * 	s_94;
	
	s_35e = (struct_35e_t *)___1e6ed0h;
	s_94 = (struct_94_t *)___1de580h;

	ecx = 0;
	while(1){

		if((edx < (int)s_35e[idx].Drug)&&(eax > (int)s_35e[idx].Drug)) ecx = 0x1;
		eax -= 60;
		edx -= 60;
		if(eax < i) break;
	}
	
	if(ecx != 0){

		if(s_35e[idx].Finished == 0){

			if((int)s_94[idx].__18 > 0) s_35e[idx].Ctrls[getCounter(4)] |= action;
		}
	}
}

// sin_b and cos_b of the direction behind the car
// the cars' positions as integers, which the AI doesn't change
static int 	car_x[4], car_y[4];

static void helper_machine_gun(float dval, float sin_b, float cos_b, int idx){

	int 			i, j, n;
	struct_35e_t * 	s_35e;
	
	s_35e = (struct_35e_t *)___1e6ed0h;

	i = (int)(s_35e[idx].XLocation+dval*sin_b);
	j = (int)(s_35e[idx].YLocation+dval*cos_b);

	n = -1;
	while(++n < (int)D(NUM_OF_CARS)){

		if(idx != n){
			
			if((dRMath_abs_i(car_x[n]-i) < 20)&&(dRMath_abs_i(car_y[n]-j) < 20)){
					
				s_35e[idx].Ctrls[getCounter(4)] |= CTRL_MACHINE_GUN;
			}
		}
	}
}

// RACE CARS AI
void race___4b62ch(int idx){

	__DWORD__ 		eax, ebx, ecx, edx;
	__BYTE__ 	b1, b2;
	int 		i, j, n, a_d;
	float 		sin_d, cos_d;
	struct_35e_t * 	s_35e;
	struct_94_t *	s_94;
	img40x40 *	carimage;
	__DWORD__ * 	ctrl;

	s_35e = (struct_35e_t *)___1e6ed0h;
	s_94 = (struct_94_t *)___1de580h;
	D(___243cfch) = 0;
	ctrl = &s_35e[idx].Ctrls[getCounter(4)];

	for(n = 0; n < (int)D(NUM_OF_CARS); n++){

		car_x[n] = (int)s_35e[n].XLocation;
		car_y[n] = (int)s_35e[n].YLocation;
	}

	// the points ahead of and behind the car are at the direction or 180
	// degrees from it, so they all come from one sine and cosine
	a_d = dRMath_turns_degf(s_35e[idx].Direction);
	sin_d = dRMath_fixf(dRMath_sin_i(a_d), 28);
	cos_d = dRMath_fixf(dRMath_cos_i(a_d), 28);

	i = (int)(s_35e[idx].XLocation-35.0f*sin_d);
	j = (int)(s_35e[idx].YLocation-35.0f*cos_d);

	n = -1;
	while(++n < (int)D(NUM_OF_CARS)){

		if(idx != n){

			ebx = i-car_x[n];
			edx = j-car_y[n];

			if((dRMath_abs_i(ebx) < 20)&&(dRMath_abs_i(edx) < 20)){

				carimage = (img40x40 *)(race_car_img(s_35e[n].ImgOffset));

				if((*carimage)[(int)edx+20][(int)ebx+20] > 3){
					
					if((s_35e[idx].__186 == 0)&&(s_35e[idx].Finished == 0)) s_35e[idx].__186 = 100;
				}
			}
		}
	}

	i = (int)(s_35e[idx].XLocation-25.0f*sin_d);
	j = (int)(s_35e[idx].YLocation-25.0f*cos_d);

	n = -1;
	while(++n < (int)getCounter(6)){

		if((dRMath_abs_i(D(___1df520h+0x10*n)-i) < 20)&&(dRMath_abs_i(D(___1df520h+0x10*n+4)-j) < 20)){

			if((s_35e[idx].__186 == 0)&&(s_35e[idx].Finished == 0)) s_35e[idx].__186 = 60;
		}
	}

	if((s_35e[idx].__182 == 0)&&((int)s_35e[idx].__192 > 5)) s_35e[idx].__182 = 100;
	if((s_35e[idx].__186 == 0)&&((int)s_35e[idx].__196 > 3)) s_35e[idx].__186 = 100;

	*ctrl = CTRL_NULL;
	
	helper_drug(560, 530, 60, CTRL_STEER_LEFT, idx);
	helper_drug(530, 500, 30, CTRL_STEER_RIGHT, idx);

	if((int)s_35e[idx].__17e >= 100){

		if((int)s_35e[idx].__17e > 148){
		
			if(s_35e[idx].__b0 > 0.5f) *ctrl |= CTRL_BRAKE;
		}

		if((int)s_35e[idx].__17e < 148) *ctrl |= CTRL_STEER_LEFT;
		if((int)s_35e[idx].__182 > 0) *ctrl = CTRL_BRAKE;
	}
	else {

		ebx = B(TRX_VAI+((int)s_35e[idx].XLocation>>0x2)+TRX_WIDTH_QTR*((int)s_35e[idx].YLocation>>0x2));

		if((0.0f < s_94[idx].__4)||(s_35e[idx].__b0 > 0.5f)){

			// 40 pixels out at 206 and 154 degrees: sin(d+206) is -sin(d+26)
			int 	a26 = a_d+(int)((26u << 24)/360), a154 = a_d+(int)((154u << 24)/360);

			i = helper00(TRX_WIDTH-1, (int)(s_35e[idx].XLocation-dRMath_fixf(40*(dRMath_sin_i(a26) >> 4), 24)));
			j = helper00(TRX_HEIGHT-1, (int)(s_35e[idx].YLocation-dRMath_fixf(40*(dRMath_cos_i(a26) >> 4), 24)));
			b1 = 0x10-B(TRX_LR1+TRX_WIDTH_QTR*j+i);
			
			i = helper00(TRX_WIDTH-1, (int)(s_35e[idx].XLocation+dRMath_fixf(40*(dRMath_sin_i(a154) >> 4), 24)));
			j = helper00(TRX_HEIGHT-1, (int)(s_35e[idx].YLocation+dRMath_fixf(40*(dRMath_cos_i(a154) >> 4), 24)));
			b2 = 0x10-B(TRX_LR1+TRX_WIDTH_QTR*j+i);

			if(s_35e[idx].__186 != 0){

				eax = TRX_OHI_DAT[ebx]+1;
				ecx = b1;
				if((int)ecx > (int)eax) *ctrl |= CTRL_STEER_RIGHT;

				eax = TRX_OHI_DAT[ebx]-1;
				ecx = b2;
				if((int)ecx < (int)eax) *ctrl |= CTRL_STEER_LEFT;
			}
			else {

				if(b1 > 9) *ctrl |= CTRL_STEER_RIGHT;
				if(b2 < 7) *ctrl |= CTRL_STEER_LEFT;
			}
		}

		if((dRMath_mod15(rand_watcom106(), 2)) == 0){

			if((int)s_35e[idx].__196 > 3){

				if(s_35e[idx].Finished == 0){

					if((int)s_94[idx].__18 > 0){
					
						*ctrl |= CTRL_STEER_RIGHT;
					}
				}
			}
		}

		if((s_94[idx].__4*TRX_DRV_DAT[ebx]) > s_35e[idx].__b0){

			if(s_35e[idx].__182 == 0) *ctrl |= CTRL_ACCELERATE;
		}

		s_35e[idx].__a8 = s_94[idx].__14*TRX_DRV_DAT[ebx+0x100];
	}

	if((int)s_35e[idx].__186 > 0){
		
		*ctrl |= CTRL_TURBO_BOOST;
		s_35e[idx].__186 -= 1;
	}

	if((int)s_35e[idx].__17e > 0) s_35e[idx].__17e -= 1;
	if((int)s_35e[idx].__182 > 0) s_35e[idx].__182 -= 1;

	i = (int)(s_35e[idx].XLocation+55.0f*sin_d);
	j = (int)(s_35e[idx].YLocation+55.0f*cos_d);

	n = -1;
	while(++n < (int)D(NUM_OF_CARS)){

		if(idx != n){

			if((dRMath_abs_i(car_x[n]-i) < 20)&&(dRMath_abs_i(car_y[n]-j) < 20)){

				if(s_35e[idx].MineTimer == 0){
				
					*ctrl |= CTRL_DROP_MINE;
					s_35e[idx].MineTimer = 350;
				}
			}
		}
	}

	if((int)s_35e[idx].MineTimer > 0) s_35e[idx].MineTimer -= 1;

	helper_machine_gun(20.0f, -sin_d, -cos_d, idx);
	helper_machine_gun(50.0f, -sin_d, -cos_d, idx);
	helper_machine_gun(80.0f, -sin_d, -cos_d, idx);

	if((s_35e[idx].__186 == 70)&&((dRMath_mod15(rand_watcom106(), 5)) == 0)) s_35e[idx].__35a = 1;
	if(s_35e[idx].__186 == 0) s_35e[idx].__35a = 0;
	if(s_35e[idx].__35a == 1) *ctrl |= CTRL_HORN;
}
