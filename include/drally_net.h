#ifndef __DRALLY_NET_H
#define __DRALLY_NET_H

#include "types.h"
#include "drally_ipx.h"
#include "netpage.h"

// __mp_data.c
extern __DWORD__ ___199f9ch;
extern __DWORD__ ___199fa0h;
extern __DWORD__ ___199fa4h_com_port_addr;
extern __DWORD__ ___199fa8h_com_port_irq;
extern __DWORD__ ___199fach;
extern __BYTE__ ___199fb6h;
extern __DWORD__ ___199fb8h;
extern __DWORD__ MP_ERROR;
extern __DWORD__ ___199fc0h;
extern __DWORD__ ___199fd0h;
extern __DWORD__ ___199fd4h;

// bss.c
extern IPX_Header * ___24cd00h[0x10];
extern IPX_EventControlBlock * ___24cd60h[0x10];
extern __POINTER__ ___24cdc0h[0x10];
extern NetPage * ___24cee0h[4];
extern IPX_Address ___24cef0h[4];
extern __BYTE__ ___24cf28h[0x1000];
extern __BYTE__ ___24df28h[0x400];
extern char ___24e328h[0xc8];
extern __DWORD__ ___24e3f0h[10];
extern __DWORD__ ___24e418h[10];
extern __DWORD__ ___24e450h;
extern IPX_EventControlBlock * ___24e45ch;
extern __POINTER__ ___24e464h;
extern __POINTER__ ___24e46ch;
extern NetPage * ___24e4ach;
extern IPX_Address * ___24e4b4h;
extern NetPage * ___24e4c0h;
extern __POINTER__ ___24e4c4h;
extern __DWORD__ CONNECTION_TYPE;
extern IPX_Header * ___24e548h;
extern IPX_EventControlBlock * ___24e54eh;
extern __POINTER__ ___24e58ah;
extern __BYTE__ ___24e596h[0xa];

void ___60a28h(__DWORD__ A1);
void ___60a84h(void);
__DWORD__ ___60b48h(__DWORD__ A1, __DWORD__ A2);
__DWORD__ ___60b60h(__DWORD__ A1, __DWORD__ A2, __POINTER__ A3, int A4);
__DWORD__ ___611c0h(__DWORD__ A1, __POINTER__ * A2);
void ___61278h(void);
__DWORD__ ___612c8h(__DWORD__ A0);
void ___61418h(__DWORD__ A1);
void ___61518h(void);
void ___6168ch(void);
void ___618c4h(void);
__DWORD__ ___61b88h(__DWORD__ A1);
__DWORD__ ___61cd0h(void);
void ___623d4h(void);
__DWORD__ ___638ech(void);
void ___63b20h(__DWORD__ A1, __DWORD__ A2);
__WORD__ swap16(__WORD__ val);

// chatbox.c
void dRChatbox_clear(void);
void dRChatbox_push(const char * line, int col);
int dRChatbox_getFont(int line);
char * dRChatbox_getLine(int line);

#endif // __DRALLY_NET_H
