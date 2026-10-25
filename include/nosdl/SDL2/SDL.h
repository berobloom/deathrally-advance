// The part of SDL that the game's portable code uses, for builds without
// SDL (the GBA, tools/replay): the byte order and the little-endian swaps,
// for little-endian CPUs.
#ifndef __NOSDL_SDL_H
#define __NOSDL_SDL_H

#define SDL_LIL_ENDIAN		1234
#define SDL_BIG_ENDIAN		4321
#define SDL_BYTEORDER		SDL_LIL_ENDIAN

#define SDL_SwapLE16(x)		(x)
#define SDL_SwapLE32(x)		(x)
#define SDL_SwapFloatLE(x)	(x)

#endif // __NOSDL_SDL_H
