#include "drally.h"
#include "drally_ipx.h"
#include "drally_net.h"


void ___61418h(__DWORD__ A1){
#if defined(DR_MULTIPLAYER)
    if(CONNECTION_TYPE == 2){

        getIPXFragment(___24e54eh, 1)->Size = 1;
        ___24e58ah[0] = A1;

        if(NovellNetWare_IPX_SendPacket(___24e54eh)) MP_ERROR = 0xca;
    }
#endif // DR_MULTIPLAYER
}
