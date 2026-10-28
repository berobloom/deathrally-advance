// gbabake: decode a Death Rally track and the race cars with the game's own
// BPA/BPK readers and write them in the layouts the GBA prototype uses.
//
//   gbabake GAMEDIR TRACK OUTDIR [TICKLOG.BIN]
//
// Writes, for track TRn:
//   track.pal    256 BGR555 colors (the track's 6-bit VGA palette)
//   track.tiles  the track image as 8bpp 8x8 tiles, row by row
//   track.mas    the track mask, one byte per pixel (wall/surface codes)
//   track.inf    width and height in tiles, then in pixels, then the 4 start positions
//                (x, y, rot), all as little-endian 32-bit values
//   cars.obj     4 cars x 96 directions, each 40x40 8bpp as 5x5 tiles, row by row
//                (index 0 is clear)
//   preview.ppm  the track and car 0's 96 frames, for checking by eye
//
// and what race_loaders_baked (src/race/race_load_baked.c) sets up a race
// from, for gba/race:
//   inf.bin      TRn-INF.BIN as it is
//   vai.bin      TRn-VAI.BPK and TRn-LR1.BPK decoded (track.mas is TRn-MAS.BPK)
//   lr1.bin
//   ski.tab      TRn-SKI.TAB, -BLO.TAB and -LIT.TAB, and VARJO.TAB
//   blo.tab
//   lit.tab
//   varjo.tab
//   drv.dat      TRn-DRV.DAT and TRn-OHI.DAT
//   ohi.dat
//   mines1a.bin  MINES1A.BPK, BLOWI.BPK and OBSTACLE.BPK decoded
//   blowi.bin
//   obstacle.bin
//   cars.img     the 13 car images (7 models, then the -S.BPK of the first 6),
//                96 frames of 40x40 each, not recolored
//   cars-all.obj the same as sprites, as in cars.obj, for each car image
//                in each of the 4 places' colors: [image][place][frame]
//
// With a race recorded on this track by the desktop game (make ticklog),
// also race.bin, the recording up to the race's start, which gba/race
// starts the race from; cars.obj then has the recorded race's cars.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "drally.h"
#include "bpa.h"

void bpk_decode2(__POINTER__ dst, __POINTER__ src);
void bpk_decode4(__DWORD__ size, __DWORD__ offset, __POINTER__ dst, __POINTER__ src);

#define CAR_FRAMES	96
#define CAR_W		40
#define CAR_TILES	(CAR_W/8)	// a frame is 5x5 tiles of the 64x64 sprite it is shown in

#pragma pack(push, 1)
typedef struct { __DWORD__ x, y, rot; } startup_t;
typedef struct {
	__DWORD__	w, h, unk0;
	startup_t	startups[4];
} inf_t;
#pragma pack(pop)

// models of the 4 cars in the prototype, as named in race___42824h.c
static const char * car_models[4] = { "LOTUS", "CAMARO", "PORCHE", "SEDAN" };
// their body colors, 6-bit VGA: red, blue, yellow, green
static const int car_colors[4][3] = { {63, 8, 4}, {8, 24, 63}, {63, 52, 4}, {8, 52, 12} };

// car n's 10 body colors from 15+10*n on, as ___4a064h_cdecl builds them
static void car_ramp(__BYTE__ * pal, int n){

	int 	i, k;

	for(i = 0; i < 5; i++){
		for(k = 0; k < 3; k++){

			pal[3*(15+10*n+i)+k] = (int)((0.18*i+0.1)*car_colors[n][k]);
			pal[3*(15+10*n+i+5)+k] = (int)(12.6*i+(1.0-0.2*i)*car_colors[n][k]);
		}
	}
}

static __POINTER__ read_entry(const char * bpa_name, const char * entry){

	BPA *		bpa;
	__POINTER__	buf;

	if(!(bpa = bpa_open(bpa_name))){ fprintf(stderr, "can't open %s\n", bpa_name); exit(1); }
	bpa_search(bpa, entry);
	if(!bpa_entry_size(bpa)){ fprintf(stderr, "%s: no %s\n", bpa_name, entry); exit(1); }
	buf = calloc(1, bpa_entry_size(bpa) + 0x10000);
	bpa_read(bpa, buf);
	bpa_close(bpa);

	return buf;
}

static __POINTER__ read_entry_size(const char * bpa_name, const char * entry, unsigned * size){

	BPA *		bpa;
	__POINTER__	buf;

	if(!(bpa = bpa_open(bpa_name))){ fprintf(stderr, "can't open %s\n", bpa_name); exit(1); }
	bpa_search(bpa, entry);
	if(!(*size = bpa_entry_size(bpa))){ fprintf(stderr, "%s: no %s\n", bpa_name, entry); exit(1); }
	buf = calloc(1, *size + 0x10000);
	bpa_read(bpa, buf);
	bpa_close(bpa);

	return buf;
}

static FILE * out(const char * dir, const char * name){

	char	path[PATH_MAX+32];
	FILE *	fd;

	snprintf(path, sizeof(path), "%s/%s", dir, name);
	if(!(fd = fopen(path, "wb"))){ perror(path); exit(1); }

	return fd;
}

// a recording's car images (its buffer 4, see race_replay.h), after it
// checked that it was made on a w x h track. Writes race.bin
static __POINTER__ read_race(const char * path, int w, int h, const char * outdir){

	FILE * 		fd;
	long 		size;
	__DWORD__ * 	log;
	__DWORD__ * 	p;
	__POINTER__ 	spins = 0;
	int 		i;

	if(!(fd = fopen(path, "rb"))){ perror(path); exit(1); }
	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	log = malloc(size);
	if(fread(log, 1, size, fd) != (size_t)size){ perror(path); exit(1); }
	fclose(fd);

	if(log[0] != 0x4c545244){ fprintf(stderr, "%s: not a recording\n", path); exit(1); }
	p = log+4+log[3];
	for(i = 0; i < 8; i++){

		if((i == 0)&&(p[0] != (__DWORD__)(w*h))){ fprintf(stderr, "%s: recorded on another track\n", path); exit(1); }
		if(i == 4) spins = (__POINTER__)(p+1);
		p += 1+(p[0]+3)/4;
	}
	p += (log[2]+3)/4;	// the globals at the start

	fd = out(outdir, "race.bin");
	fwrite(log, 4, p-log, fd);
	fwrite((__DWORD__ []){ 0xffffffff }, 4, 1, fd);
	fclose(fd);

	return spins;
}

static void write_file(const char * dir, const char * name, const void * p, unsigned size){

	FILE * 	fd = out(dir, name);

	fwrite(p, 1, size, fd);
	fclose(fd);
}

// an entry as it is
static void bake_entry(const char * bpa_name, const char * entry, const char * dir, const char * name, unsigned size){

	unsigned 	n;
	__POINTER__ 	src = read_entry_size(bpa_name, entry, &n);

	if(size&&(n != size)){ fprintf(stderr, "%s: %s is %u bytes, not %u\n", bpa_name, entry, n, size); exit(1); }
	write_file(dir, name, src, n);
	free(src);
}

// an ENGINE.BPA picture decoded
static void bake_bpk2(const char * entry, const char * dir, const char * name, unsigned size){

	__POINTER__ 	src = read_entry("ENGINE.BPA", entry), dst = calloc(1, size);

	bpk_decode2(dst, src);
	write_file(dir, name, dst, size);
	free(src);
	free(dst);
}

// 96 frames of a car as 8bpp 8x8 tiles, 5x5 a frame
static void put_frames(FILE * fd, const __BYTE__ * spins){

	__BYTE__ 	obj[CAR_W*CAR_W];
	int 		f, x, y;

	for(f = 0; f < CAR_FRAMES; f++){

		for(y = 0; y < CAR_W; y++){
			for(x = 0; x < CAR_W; x++){

				obj[64*((y/8)*CAR_TILES+x/8)+8*(y%8)+x%8] = spins[f*CAR_W*CAR_W+y*CAR_W+x];
			}
		}
		fwrite(obj, 1, sizeof(obj), fd);
	}
}

// what race_loaders_baked needs besides track.mas
static void bake_setup(int track, int w, int h, const char * outdir){

	static const char * 	models[7] = { "KUPLA", "PICKUP", "SEDAN", "CAMARO", "PORCHE", "LOTUS", "SPECIAL" };
	char 		bpa_name[16], entry[32];
	int 		wq = w >> 2, hq = h >> 2, v, n, i;
	__POINTER__ 	src, buf, cars, car;
	FILE * 		fd;

	snprintf(bpa_name, sizeof(bpa_name), "TR%d.BPA", track);

	snprintf(entry, sizeof(entry), "TR%d-INF.BIN", track);
	bake_entry(bpa_name, entry, outdir, "inf.bin", 0);

	buf = calloc(1, wq*hq);
	snprintf(entry, sizeof(entry), "TR%d-VAI.BPK", track);
	src = read_entry(bpa_name, entry);
	bpk_decode4(wq*hq, 0x30a, buf, src);
	write_file(outdir, "vai.bin", buf, wq*hq);
	free(src);
	snprintf(entry, sizeof(entry), "TR%d-LR1.BPK", track);
	src = read_entry(bpa_name, entry);
	bpk_decode4(wq*hq, 0x30a, buf, src);
	write_file(outdir, "lr1.bin", buf, wq*hq);
	free(src);
	free(buf);

	snprintf(entry, sizeof(entry), "TR%d-SKI.TAB", track);
	bake_entry(bpa_name, entry, outdir, "ski.tab", 0x100);
	snprintf(entry, sizeof(entry), "TR%d-BLO.TAB", track);
	bake_entry(bpa_name, entry, outdir, "blo.tab", 0x100);
	snprintf(entry, sizeof(entry), "TR%d-LIT.TAB", track);
	bake_entry(bpa_name, entry, outdir, "lit.tab", 0x100);
	bake_entry("ENGINE.BPA", "VARJO.TAB", outdir, "varjo.tab", 0x100);
	snprintf(entry, sizeof(entry), "TR%d-DRV.DAT", track);
	bake_entry(bpa_name, entry, outdir, "drv.dat", 0x800);
	snprintf(entry, sizeof(entry), "TR%d-OHI.DAT", track);
	bake_entry(bpa_name, entry, outdir, "ohi.dat", 0x400);

	bake_bpk2("MINES1A.BPK", outdir, "mines1a.bin", 0x140);
	bake_bpk2("BLOWI.BPK", outdir, "blowi.bin", 0x600);
	bake_bpk2("OBSTACLE.BPK", outdir, "obstacle.bin", 0x800);

	cars = calloc(13, CAR_FRAMES*CAR_W*CAR_W);
	for(v = 0; v < 13; v++){

		snprintf(entry, sizeof(entry), "%s%s", models[v%7], (v < 7) ? ".BPK" : "-S.BPK");
		src = read_entry("ENGINE.BPA", entry);
		bpk_decode2(cars+v*CAR_FRAMES*CAR_W*CAR_W, src);
		free(src);
	}
	write_file(outdir, "cars.img", cars, 13*CAR_FRAMES*CAR_W*CAR_W);

	// recolored as race___42824h() does for the car in place n
	fd = out(outdir, "cars-all.obj");
	car = malloc(CAR_FRAMES*CAR_W*CAR_W);
	for(v = 0; v < 13; v++){
		for(n = 0; n < 4; n++){

			memcpy(car, cars+v*CAR_FRAMES*CAR_W*CAR_W, CAR_FRAMES*CAR_W*CAR_W);
			for(i = 0; i < CAR_FRAMES*CAR_W*CAR_W; i++) if((car[i] >= 0xf)&&(car[i] <= 0x18)) car[i] += 0xa*n;
			put_frames(fd, car);
		}
	}
	fclose(fd);
	free(car);
	free(cars);
}

static void put32(FILE * fd, __DWORD__ v){

	fputc(v, fd); fputc(v >> 8, fd); fputc(v >> 16, fd); fputc(v >> 24, fd);
}

int main(int argc, char * argv[]){

	char		bpa_name[16], entry[32], outdir[PATH_MAX];
	__POINTER__	src, ima, mas, spins, race_spins = 0;
	__BYTE__	pal[0x300];
	inf_t *		inf;
	int 		track, w, h, tw, th, tx, ty, x, y, n, f, i;
	FILE *		fd;

	if((argc != 4)&&(argc != 5)){ fprintf(stderr, "usage: gbabake GAMEDIR TRACK OUTDIR [TICKLOG.BIN]\n"); return 1; }
	track = atoi(argv[2]);
	if(!realpath(argv[3], outdir)){ perror(argv[3]); return 1; }
	if(argc == 5){

		char 	race[PATH_MAX];

		if(!realpath(argv[4], race)){ perror(argv[4]); return 1; }
		argv[4] = strdup(race);
	}
	if(chdir(argv[1])){ perror(argv[1]); return 1; }

	snprintf(bpa_name, sizeof(bpa_name), "TR%d.BPA", track);

	snprintf(entry, sizeof(entry), "TR%d-INF.BIN", track);
	inf = (inf_t *)read_entry(bpa_name, entry);
	w = inf->w; h = inf->h;
	tw = (w+7)/8; th = (h+7)/8;

	snprintf(entry, sizeof(entry), "TR%d-IMA.BPK", track);
	src = read_entry(bpa_name, entry);
	bpk_decode4(0x300, 0xa, pal, src);
	ima = calloc(1, w*h);
	bpk_decode4(w*h, 0x30a, ima, src);
	free(src);
	for(n = 0; n < 4; n++) car_ramp(pal, n);

	fd = out(outdir, "track.pal");
	for(n = 0; n < 0x100; n++){

		__WORD__ c = (pal[3*n] >> 1) | ((pal[3*n+1] >> 1) << 5) | ((pal[3*n+2] >> 1) << 10);
		fputc(c, fd); fputc(c >> 8, fd);
	}
	fclose(fd);

	fd = out(outdir, "track.tiles");
	for(ty = 0; ty < th; ty++){
		for(tx = 0; tx < tw; tx++){
			for(y = 0; y < 8; y++){
				for(x = 0; x < 8; x++){

					int px = 8*tx+x, py = 8*ty+y;
					fputc(((px < w)&&(py < h)) ? ima[py*w+px] : 0, fd);
				}
			}
		}
	}
	fclose(fd);

	snprintf(entry, sizeof(entry), "TR%d-MAS.BPK", track);
	src = read_entry(bpa_name, entry);
	mas = calloc(1, w*h);
	bpk_decode4(w*h, 0x30a, mas, src);
	free(src);

	fd = out(outdir, "track.mas");
	fwrite(mas, 1, w*h, fd);
	fclose(fd);

	fd = out(outdir, "track.inf");
	put32(fd, tw); put32(fd, th); put32(fd, w); put32(fd, h);
	for(n = 0; n < 4; n++){ put32(fd, inf->startups[n].x); put32(fd, inf->startups[n].y); put32(fd, inf->startups[n].rot); }
	fclose(fd);

	bake_setup(track, w, h, outdir);
	if(argc == 5) race_spins = read_race(argv[4], w, h, outdir);

	fd = out(outdir, "cars.obj");
	spins = calloc(1, CAR_FRAMES*CAR_W*CAR_W);
	for(n = 0; n < 4; n++){

		if(race_spins){

			// recolored already
			memcpy(spins, race_spins+n*CAR_FRAMES*CAR_W*CAR_W, CAR_FRAMES*CAR_W*CAR_W);
		}
		else {

			snprintf(entry, sizeof(entry), "%s.BPK", car_models[n]);
			src = read_entry("ENGINE.BPA", entry);
			bpk_decode2(spins, src);
			free(src);

			// same recoloring as race___42824h.c: car n gets its own body colors
			for(i = 0; i < CAR_FRAMES*CAR_W*CAR_W; i++) if((spins[i] >= 0xf)&&(spins[i] <= 0x18)) spins[i] += 0xa*n;
		}

		put_frames(fd, spins);

		if(n == 0){

			// preview: the track, then car 0's frames in rows of 24 below it
			FILE * pv = out(outdir, "preview.ppm");
			int pw = (w > 24*CAR_W) ? w : 24*CAR_W, ph = h+4*CAR_W;
			fprintf(pv, "P6 %d %d 63\n", pw, ph);
			for(y = 0; y < ph; y++){
				for(x = 0; x < pw; x++){

					int c = 0;
					if(y < h){ if(x < w) c = ima[y*w+x]; }
					else {
						int fy = (y-h)/CAR_W, fx = x/CAR_W;
						if(fx < 24) c = spins[(fy*24+fx)*CAR_W*CAR_W+((y-h)%CAR_W)*CAR_W+x%CAR_W];
					}
					fwrite(pal+3*c, 1, 3, pv);
				}
			}
			fclose(pv);
		}
	}
	fclose(fd);

	printf("TR%d: %dx%d (%dx%d tiles), start (%u,%u) rot %u\n", track, w, h, tw, th,
		inf->startups[0].x, inf->startups[0].y, inf->startups[0].rot);
	return 0;
}
