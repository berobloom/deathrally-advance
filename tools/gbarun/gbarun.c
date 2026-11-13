// gbarun: runs a GBA ROM headless in libmgba, pressing buttons, writing and
// reading memory and saving screenshots at given frames, and prints what
// the ROM logs (gba/race's say(), through mGBA's debug registers).
//
//   gbarun [-s SAVE] [-a SOUND.wav] [-p TICK:PHASE:SLOW:OUT] ROM FRAMES [SCRIPT]
//
// -s keeps the cartridge's save memory in the file SAVE, made if it
// isn't there, as a real cartridge's battery would. -a records what it
// plays, 32768 Hz stereo.
//
// -p profiles a benchmark build: it runs an instruction at a time and
// adds each one's cycles to its address, apart for the ticks listed in
// the file SLOW (the numbers gba/race's "BENCH slow tick N" logs) and for
// the others, and by what the ROM says it's doing. TICK is the address of
// the ROM's tick counter (bench_ticks in the ROM's ELF), which is N-1
// while tick N runs, PHASE of its bench_phase (0-3). OUT gets the ticks'
// counts, then a line for each address, "slow|other PHASE ADDR CYCLES";
// tools/gbarun/profile.py names the functions.
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
// GBARUN_BIOS=gba_bios.bin runs it with the real BIOS.
//
// Addresses come from the ROM's map, gba/race/build/drrace.map. With no
// SCRIPT it just runs, as for the benchmark builds. See shop.txt.

#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <mgba/core/timing.h>
#include <mgba-util/vfs.h>
#include <fcntl.h>
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

#define SOUND_RATE		32768

static void put32le(FILE * f, uint32_t v){ fputc(v, f); fputc(v >> 8, f); fputc(v >> 16, f); fputc(v >> 24, f); }

// a 16-bit stereo WAV's header, for n frames of samples
static void wav_header(FILE * f, uint32_t n){

	fseek(f, 0, SEEK_SET);
	fwrite("RIFF", 1, 4, f); put32le(f, 36+4*n);
	fwrite("WAVEfmt ", 1, 8, f); put32le(f, 16);
	put32le(f, 1|(2 << 16));		// PCM, 2 channels
	put32le(f, SOUND_RATE); put32le(f, 4*SOUND_RATE);
	put32le(f, 4|(16 << 16));		// 4 bytes a frame, 16 bits
	fwrite("data", 1, 4, f); put32le(f, 4*n);
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

// -p's: cycles by address, halfword by halfword, in the ROM's first 4 MB,
// EWRAM and IWRAM, the rest together
#define PROF_ROM 		(4 << 20)
#define PROF_N 			((PROF_ROM+(256 << 10)+(32 << 10))/2+2)	// and the BIOS, and the rest
static uint64_t 	* prof[2][4];
static unsigned char 	slow_tick[1 << 16];

static unsigned prof_slot(uint32_t pc){

	if((pc >= 0x08000000)&&(pc < 0x08000000+PROF_ROM)) return (pc-0x08000000)/2;
	if((pc >= 0x02000000)&&(pc < 0x02040000)) return (PROF_ROM+pc-0x02000000)/2;
	if((pc >= 0x03000000)&&(pc < 0x03008000)) return (PROF_ROM+(256 << 10)+pc-0x03000000)/2;
	if(pc < 0x4000) return PROF_N-2;	// the BIOS together

	return PROF_N-1;
}

static uint32_t prof_addr(unsigned k){

	if(k == PROF_N-1) return 0;
	if(k == PROF_N-2) return 2;
	k *= 2;
	if(k < PROF_ROM) return 0x08000000+k;
	if(k < PROF_ROM+(256 << 10)) return 0x02000000+k-PROF_ROM;

	return 0x03000000+k-PROF_ROM-(256 << 10);
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
	const char 	* save = NULL, * prof_out = NULL;
	uint32_t 	tick_addr = 0, phase_addr = 0, ticks[2] = { 0, 0 };
	FILE 		* wav = NULL;
	uint32_t 	wav_n = 0;

	while((argc > 2)&&(!strcmp(argv[1], "-s")||!strcmp(argv[1], "-a")||!strcmp(argv[1], "-p"))){

		if(argv[1][1] == 'p'){

			static char 	arg[512];
			char 		* phase, * slow, * out;
			FILE 		* sf;
			unsigned 	n, k;

			snprintf(arg, sizeof(arg), "%s", argv[2]);
			if(!(phase = strchr(arg, ':'))||!(slow = strchr(phase+1, ':'))||!(out = strchr(slow+1, ':'))){

				fprintf(stderr, "gbarun: -p TICK:PHASE:SLOW:OUT\n");
				return 1;
			}
			*phase++ = 0;
			*slow++ = 0;
			*out++ = 0;
			tick_addr = strtoul(arg, NULL, 16);
			phase_addr = strtoul(phase, NULL, 16);
			prof_out = out;
			if(!(sf = fopen(slow, "r"))){

				fprintf(stderr, "gbarun: can't open %s\n", slow);
				return 1;
			}
			while(fscanf(sf, "%u", &n) == 1) if(n < sizeof(slow_tick)) slow_tick[n] = 1;
			fclose(sf);
			for(k = 0; k < 8; k++) prof[k/4][k%4] = calloc(PROF_N, sizeof(uint64_t));
		}
		else if(argv[1][1] == 's') save = argv[2];
		else if(!(wav = fopen(argv[2], "wb"))){

			fprintf(stderr, "gbarun: can't write %s\n", argv[2]);
			return 1;
		}
		argv += 2;
		argc -= 2;
	}
	if(argc < 3){

		fprintf(stderr, "usage: gbarun [-s SAVE] [-a SOUND.wav] ROM FRAMES [SCRIPT]\n");
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
	// GBARUN_BIOS=FILE: the real BIOS, which a cartridge on the hardware
	// runs with, instead of mGBA's own stand-in for its calls
	if(getenv("GBARUN_BIOS")){

		struct VFile * 	bios = VFileOpen(getenv("GBARUN_BIOS"), O_RDONLY);

		if(!bios||!core->loadBIOS(core, bios, 0)){

			fprintf(stderr, "gbarun: can't use %s for the BIOS\n", getenv("GBARUN_BIOS"));
			return 1;
		}
		mCoreConfigSetIntValue(&core->config, "useBios", 1);
		mCoreConfigSetIntValue(&core->config, "skipBios", 1);
		core->reloadConfigOption(core, "useBios", NULL);
	}
	core->reset(core);
	if(wav){

		wav_header(wav, 0);
		core->setAudioBufferSize(core, 2048);
		blip_set_rates(core->getAudioChannel(core, 0), core->frequency(core), SOUND_RATE);
		blip_set_rates(core->getAudioChannel(core, 1), core->frequency(core), SOUND_RATE);
	}

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
		if(prof_out){

			// an instruction at a time, its cycles to where it is
			uint32_t 	frame = core->frameCounter(core), tick = core->rawRead32(core, tick_addr, -1);
			uint64_t 	t = mTimingGlobalTime(core->timing);

			while(core->frameCounter(core) == frame){

				int32_t 	pc = 0;
				uint32_t 	now_tick;
				uint64_t 	t1;

				core->readRegister(core, "pc", &pc);
				core->step(core);
				t1 = mTimingGlobalTime(core->timing);
				prof[(tick+1 < sizeof(slow_tick))&&slow_tick[tick+1]][core->rawRead32(core, phase_addr, -1)&3][prof_slot((uint32_t)pc)] += t1-t;
				t = t1;
				now_tick = core->rawRead32(core, tick_addr, -1);
				if(now_tick != tick){

					ticks[(tick+1 < sizeof(slow_tick))&&slow_tick[tick+1]]++;
					tick = now_tick;
				}
			}
		}
		else core->runFrame(core);
		if(wav){

			short 	buf[2*4096];
			int 	n = blip_samples_avail(core->getAudioChannel(core, 0));

			if(n > 4096) n = 4096;
			blip_read_samples(core->getAudioChannel(core, 0), buf, n, 1);
			blip_read_samples(core->getAudioChannel(core, 1), buf+1, n, 1);
			fwrite(buf, 4, n, wav);
			wav_n += n;
		}
	}
	if(wav){

		wav_header(wav, wav_n);
		fclose(wav);
	}

	if(prof_out){

		FILE 		* pf = fopen(prof_out, "w");
		unsigned 	k;
		int 		w2, ph;

		if(!pf){ fprintf(stderr, "gbarun: can't write %s\n", prof_out); return 1; }
		fprintf(pf, "ticks %u %u\n", ticks[1], ticks[0]);
		for(w2 = 0; w2 < 2; w2++) for(ph = 0; ph < 4; ph++) for(k = 0; k < PROF_N; k++) if(prof[w2][ph][k]) fprintf(pf, "%s %d %08x %llu\n", w2 ? "slow" : "other", ph, prof_addr(k), (unsigned long long)prof[w2][ph][k]);
		fclose(pf);
	}
	core->deinit(core);
	free(screen);

	return 0;
}
