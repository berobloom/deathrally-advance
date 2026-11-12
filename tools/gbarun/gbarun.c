// gbarun: runs a GBA ROM headless in libmgba, pressing buttons, writing and
// reading memory and saving screenshots at given frames, and prints what
// the ROM logs (gba/race's say(), through mGBA's debug registers).
//
//   gbarun [-s SAVE] ROM FRAMES [SCRIPT]
//
// -s keeps the cartridge's save memory in the file SAVE, made if it
// isn't there, as a real cartridge's battery would.
//
// SCRIPT, one command a line, # for comments:
//
//   FRAME KEYS[:N]          hold KEYS for N frames (2 if not given), then
//                           let go. A B L R U D (the arrows), S Start,
//                           s Select, l r the shoulders, - none
//   FRAME shot FILE.png     save the screen
//   FRAME poke32 ADDR=VAL   write a word, ADDR hex, VAL decimal
//   FRAME peek32 ADDR       print a word
//
// Addresses come from the ROM's map, gba/race/build/drrace.map. With no
// SCRIPT it just runs, as for the benchmark builds. See shop.txt.

#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define MAX_EVENTS 		4096

static struct {
	int 	frame;
	char 	what[16];
	char 	arg[256];
} ev[MAX_EVENTS];

// only the ROM's own log, not mGBA's
static void on_log(struct mLogger * l, int category, enum mLogLevel level, const char * fmt, va_list ap){

	const char * 	name = mLogCategoryName(category);

	(void)l; (void)level;
	if(!name||strcmp(name, "GBA Debug")) return;
	vprintf(fmt, ap);
	putchar('\n');
	fflush(stdout);
}

static void put32(FILE * f, uint32_t v){

	fputc(v >> 24, f); fputc(v >> 16, f); fputc(v >> 8, f); fputc(v, f);
}

static void chunk(FILE * f, const char * type, const unsigned char * data, uint32_t n){

	// crc32() with a NULL buffer starts over at 0, so not for IEND's none
	uint32_t 	crc = crc32(0, (const unsigned char *)type, 4);

	if(n) crc = crc32(crc, data, n);

	put32(f, n);
	fwrite(type, 1, 4, f);
	fwrite(data, 1, n, f);
	put32(f, crc);
}

// the screen as an RGB PNG; mGBA's colors are XBGR, red in the low byte
static int save_png(const char * path, const color_t * px, unsigned w, unsigned h){

	static const unsigned char 	sig[8] = { 137, 'P', 'N', 'G', 13, 10, 26, 10 };
	unsigned char 			ihdr[13] = { 0 };
	size_t 				raw_n = (size_t)h*(1+3*w);
	unsigned char 			* raw = malloc(raw_n), * z;
	uLongf 				z_n = compressBound(raw_n);
	unsigned 			x, y;
	FILE 				* f;

	z = malloc(z_n);
	for(y = 0; y < h; y++){

		unsigned char 	* row = raw+y*(1+3*w);

		row[0] = 0;
		for(x = 0; x < w; x++){

			uint32_t 	c = px[y*w+x];

			row[1+3*x] = c; row[2+3*x] = c >> 8; row[3+3*x] = c >> 16;
		}
	}
	compress2(z, &z_n, raw, raw_n, 6);
	ihdr[0] = w >> 24; ihdr[1] = w >> 16; ihdr[2] = w >> 8; ihdr[3] = w;
	ihdr[4] = h >> 24; ihdr[5] = h >> 16; ihdr[6] = h >> 8; ihdr[7] = h;
	ihdr[8] = 8;		// bits per channel
	ihdr[9] = 2;		// RGB
	if(!(f = fopen(path, "wb"))){

		free(raw); free(z);
		return 0;
	}
	fwrite(sig, 1, 8, f);
	chunk(f, "IHDR", ihdr, 13);
	chunk(f, "IDAT", z, z_n);
	chunk(f, "IEND", NULL, 0);
	fclose(f);
	free(raw); free(z);

	return 1;
}

static uint32_t parse_keys(const char * k){

	uint32_t 	keys = 0;

	for(; *k&&(*k != ':'); k++) switch(*k){
	case 'A': keys |= 1 << 0; break;
	case 'B': keys |= 1 << 1; break;
	case 's': keys |= 1 << 2; break;
	case 'S': keys |= 1 << 3; break;
	case 'R': keys |= 1 << 4; break;
	case 'L': keys |= 1 << 5; break;
	case 'U': keys |= 1 << 6; break;
	case 'D': keys |= 1 << 7; break;
	case 'r': keys |= 1 << 8; break;
	case 'l': keys |= 1 << 9; break;
	default: break;
	}

	return keys;
}

static int read_script(const char * path){

	char 	line[300];
	int 	n = 0;
	FILE 	* f = fopen(path, "r");

	if(!f){

		fprintf(stderr, "gbarun: can't open %s\n", path);
		exit(1);
	}
	while(fgets(line, sizeof(line), f)&&(n < MAX_EVENTS)){

		char 	* hash = strchr(line, '#');

		if(hash) *hash = 0;
		ev[n].arg[0] = 0;
		if(sscanf(line, "%d %15s %255s", &ev[n].frame, ev[n].what, ev[n].arg) >= 2) n++;
	}
	fclose(f);

	return n;
}

int main(int argc, char ** argv){

	struct mLogger 	logger = { .log = on_log };
	struct mCore 	* core;
	color_t 	* screen;
	unsigned 	w, h;
	int 		frames, n_ev = 0, f, i, release = -1;
	const char 	* save = NULL;

	if((argc > 2)&&!strcmp(argv[1], "-s")){

		save = argv[2];
		argv += 2;
		argc -= 2;
	}
	if(argc < 3){

		fprintf(stderr, "usage: gbarun [-s SAVE] ROM FRAMES [SCRIPT]\n");
		return 1;
	}
	frames = atoi(argv[2]);
	if(argc > 3) n_ev = read_script(argv[3]);

	mLogSetDefaultLogger(&logger);
	if(!(core = mCoreFind(argv[1]))){

		fprintf(stderr, "gbarun: %s isn't a ROM mGBA knows\n", argv[1]);
		return 1;
	}
	core->init(core);
	mCoreInitConfig(core, NULL);
	core->desiredVideoDimensions(core, &w, &h);
	screen = calloc(w*h, sizeof(color_t));
	core->setVideoBuffer(core, screen, w);
	if(!mCoreLoadFile(core, argv[1])){

		fprintf(stderr, "gbarun: can't load %s\n", argv[1]);
		return 1;
	}
	if(save&&!mCoreLoadSaveFile(core, save, false)){

		fprintf(stderr, "gbarun: can't use %s for the save\n", save);
		return 1;
	}
	core->reset(core);

	for(f = 0; f < frames; f++){

		if(f == release) core->setKeys(core, 0);
		for(i = 0; i < n_ev; i++){

			unsigned 	a, v;
			const char 	* colon;

			if(ev[i].frame != f) continue;
			if(!strcmp(ev[i].what, "shot")){

				if(!save_png(ev[i].arg, screen, w, h)) fprintf(stderr, "gbarun: can't write %s\n", ev[i].arg);
			}
			else if(!strcmp(ev[i].what, "poke32")){

				if(sscanf(ev[i].arg, "%x=%u", &a, &v) == 2) core->rawWrite32(core, a, -1, v);
			}
			else if(!strcmp(ev[i].what, "peek32")){

				if(sscanf(ev[i].arg, "%x", &a) == 1) printf("gbarun: frame %d: [%08x] = %u\n", f, a, core->rawRead32(core, a, -1));
			}
			else {

				core->setKeys(core, parse_keys(ev[i].what));
				colon = strchr(ev[i].what, ':');
				release = f+(colon ? atoi(colon+1) : 2);
			}
		}
		core->runFrame(core);
	}

	core->deinit(core);
	free(screen);

	return 0;
}
