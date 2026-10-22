#include "drally.h"
#include "netpage.h"
#include "drally_net.h"


__DWORD__ __GET_TIMER_TICKS(void);

__DWORD__ ___60b60h(__DWORD__ A1, __DWORD__ A2, __POINTER__ A3, int A4){

    __DWORD__   eax, ebx, edx, edi;
    __BYTE__    esp[0x18];


	D(esp+0xc) = 0;

	npg_writeb(___24e4c0h, 0xd3);
	npg_writeb(___24e4c0h, A1);
	npg_writeb(___24e4c0h, ___199fb6h);
	npg_writeb(___24e4c0h, ___199fb8h);
	npg_writeb(___24e4c0h, ___199fb8h>>8);
	npg_writeb(___24e4c0h, ___199fb8h>>0x10);
	npg_writeb(___24e4c0h, ___199fb8h>>0x18);

	eax = -1;
	while(++eax < A4) npg_writeb(___24e4c0h, B(A3+eax));

	npg_writeb(___24e4c0h, 0xff);
	___6168ch();

	if(___199fb6h != A2){

		if(npg_done(___24e4ach)) return (D(esp+0xc) != 0xffffffff) ? ___199fb6h : 0xff;

		while(1){

			if(___199fb6h == A2) return (D(esp+0xc) != 0xffffffff) ? ___199fb6h : 0xff;
			if(npg_stopb(___24e4ach, 0xd3)) break;
			if(npg_done(___24e4ach)) return (D(esp+0xc) != 0xffffffff) ? ___199fb6h : 0xff;
		}

		___199fd0h = __GET_TIMER_TICKS();

		edx = 0;
		while(1){

			if((npg_peekb(___24e4ach, A4+7) == 0xff)||(edx != 0)){

				if(edx != 1){
						
					if(npg_peekb(___24e4ach, 1) != A1){

						npg_skipb(___24e4ach, A4+8);
					}
					else {
							
						ebx = npg_peekb(___24e4ach, 3);
						ebx += (npg_peekb(___24e4ach, 4)<<8);
						ebx += (npg_peekb(___24e4ach, 5)<<0x10);
						ebx += (npg_peekb(___24e4ach, 6)<<0x18);
						edi = 0;

						eax = -1;
						while(++eax < 4){
							
							if(ebx == ___24e3f0h[eax]) edi = 1;
						}

						if(edi == 0){

							___24e3f0h[___199fb6h] = ebx;

							eax = -1;
							while(++eax < A4){

								B(___24cf28h+(___199fb6h<<0xa)+eax) = npg_peekb(___24e4ach, eax+7);
							}

							___199fb6h++;
						}

						eax = -1;
						while(++eax < 4){

							if(ebx == ___24e3f0h[eax]){

								B(___24e596h+eax) = npg_peekb(___24e4ach, 2);
								break;
							}
						}

						eax = -1;
						while(++eax < (A4+8)) npg_override(___24e4ach, 0);
					}
				}

				if(npg_done(___24e4ach)) return (D(esp+0xc) != 0xffffffff) ? ___199fb6h : 0xff;

				while(1){

					if(___199fb6h == A2) return (D(esp+0xc) != 0xffffffff) ? ___199fb6h : 0xff;
					if(npg_stopb(___24e4ach, 0xd3)) break;
					if(npg_done(___24e4ach)) return (D(esp+0xc) != 0xffffffff) ? ___199fb6h : 0xff;
				}

				___199fd0h = __GET_TIMER_TICKS();
				edx = 0;
			}
			else {

				___6168ch();
				
				eax = ___199fd0h+5;
				if(eax <= __GET_TIMER_TICKS()){
						
					edx = 1;
					npg_skipb(___24e4ach, 1);
				}
			}
		} // w
	}
	else {

		edi = ___24e3f0h[0];

		eax = 0;
		while(++eax < ___199fb6h){
		
			if((int)edi > (int)___24e3f0h[eax]) edi = ___24e3f0h[eax];
		}

        if(edi == ___199fb8h){

			edi = 0;
			edx = ___199fb6h;

			eax = -1;
			while(++eax < edx){

				if(___199fb8h != ___24e3f0h[eax]){
				
					if((B(___24e596h+eax) = ___199fb6h) != 0) edi++;
				}
			}

			edx--;

			if(edi == edx){

				edx = __GET_TIMER_TICKS()+2;
				while(edx > __GET_TIMER_TICKS());

				npg_writeb(___24e4c0h, 0xc5);
				npg_writeb(___24e4c0h, A1);
				npg_writeb(___24e4c0h, ___199fb6h);
				npg_writeb(___24e4c0h, ___199fb8h);
				npg_writeb(___24e4c0h, ___199fb8h>>8);
				npg_writeb(___24e4c0h, ___199fb8h>>0x10);
				npg_writeb(___24e4c0h, ___199fb8h>>0x18);

				eax = -1;
				while(++eax < A4) npg_writeb(___24e4c0h, B(A3+eax));

				npg_writeb(___24e4c0h, 0xff);
				___6168ch();
				D(esp+0xc) = 0xffffffff;
			}

			return (D(esp+0xc) != 0xffffffff) ? ___199fb6h : 0xff;
		}

		while(1){

			if(npg_done(___24e4ach)||(D(esp+0xc) == 0xffffffff)) return (D(esp+0xc) != 0xffffffff) ? ___199fb6h : 0xff;
			if(npg_stopb(___24e4ach, 0xc5)) break;
		}

		___199fd0h = __GET_TIMER_TICKS();

		edx = 0;
		while(1){

			if((npg_peekb(___24e4ach, A4+7) == 0xff)||(edx != 0)){

				if(edx != 1){

					if(npg_peekb(___24e4ach, 1)  != A1){

						npg_skipb(___24e4ach, A4+8);
					}
					else {

						eax = -1;
						while(++eax < (A4+8)) npg_override(___24e4ach, 0);

						D(esp+0xc) = 0xffffffff;
					}
				}

				while(1){

					if(npg_done(___24e4ach)||(D(esp+0xc) == 0xffffffff)) return (D(esp+0xc) != 0xffffffff) ? ___199fb6h : 0xff;
					if(npg_stopb(___24e4ach, 0xc5)) break;
				}

				___199fd0h = __GET_TIMER_TICKS();
				edx = 0;
			}
			else {

				___6168ch();

				eax = ___199fd0h+5;
				if(eax <= __GET_TIMER_TICKS()){

					edx = 1;
					npg_skipb(___24e4ach, 1);
				}
			}
		}
	}
}
