// The part of SDL that the game's portable code uses, for GBA builds: the
// byte order and the little-endian swaps, which do nothing on the GBA.
#ifndef __GBA_SDL_H
#define __GBA_SDL_H

#define SDL_LIL_ENDIAN		1234
#define SDL_BIG_ENDIAN		4321
#define SDL_BYTEORDER		SDL_LIL_ENDIAN

#define SDL_SwapLE16(x)		(x)
#define SDL_SwapLE32(x)		(x)
#define SDL_SwapFloatLE(x)	(x)

#endif // __GBA_SDL_H
