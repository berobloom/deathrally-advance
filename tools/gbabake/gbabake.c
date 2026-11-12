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
//   sprites.bin  the race's pictures drawn over the track: race_sprite_files[]
//                decoded, one after the other
//   lamps.obj    the start lights' 3 frames and the finish flag's 64 in it, as
//                the tiles of a 64x64 and a 32x64 sprite, 6 KB a frame
//   hud.bin      the HUD box's pictures from IBFILES.BPA, RACE_HUD_* in
//                drally_race.h
//   trN.s3m      the track's music, TRn-MUS.CMF decoded, for mmutil
//   men.s3m      the menus' music, MEN-MUS.CMF, the same way
//   efeNN.wav    the race's sound effects, GEN-EFE.CMF's instruments 1, 2, ...:
//                each one's first sample, 8-bit, at its base rate, with its loop
//   sfx.bin      their base volumes, 0-64, one byte each, for effect 1 on
//   menNN.wav    the menus' sound effects, MEN-SAM.CMF's, the same way, and
//   men.bin      their volumes
//   mines1a.bin  MINES1A.BPK, BLOWI.BPK and OBSTACLE.BPK decoded
//   blowi.bin
//   obstacle.bin
//   carcol.pal   MENU.BPA's CARCOL.PAL and COPPER.PAL, the racers' car colors
//   copper.pal   as the career's ___33010h_cdecl() picks them, see career.c
//   menu.pal     MENU.BPA's MENU.PAL, the menus' colors, as track.pal
//   pictures.obj the events' pictures, halved, and the loan shark's, each a
//                64x64 sprite's 8bpp tiles in menu.pal's colors
//   faces.obj    the drivers' faces, 64x64, the same way
//   cars.img     the 13 car images (7 models, then the -S.BPK of the first 6),
//                96 frames of 40x40 each, not recolored
//   cars.shp     the shapes the race code asks of them, race_car_shapes_t
//                for each car image
//   cars-all.obj the same as sprites, as in cars.obj, for each car image
//                in each of the 4 places' colors: [image][place][frame]
//
// and for the track reversed, turned round by 180 degrees as race___4af3ch()
// turns it, which race_loaders_baked can't do to the data in ROM. TR0, which
// the game never reverses (it has no TR0-FLIP.PAL), gets empty ones:
//   track-rev.pal  the reversed track's colors, TRn-FLIP.PAL, as track.pal
//   rev.mas      track.mas, vai.bin and lr1.bin reversed
//   rev.vai
//   rev.lr1
//   obst-rev.bin OBST_REV.BPK decoded, the obstacles race___42824h() loads instead
//   cars-rev.img cars.img turned with it: frame k is frame k+48 upside down
//   cars-rev.shp their shapes
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
#include "drally_race.h"

void dREncryption_decodeCMF(__POINTER__ p, size_t size);
__DWORD__ ___71b70h_cdecl(__DWORD__ period);
__DWORD__ XM_getPeriod(int Note, int FineTune);

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

static void write_file(const char * dir, const char * name, const void * p, unsigned size);

// whether the track has a reversed version, which the game gives its own colors
static int reversible;

// a map turned round by 180 degrees: its bytes in the opposite order, or
// nothing for a track that isn't reversed
static void write_reversed(const char * dir, const char * name, const __BYTE__ * p, unsigned size){

	__BYTE__ * 	r = malloc(size);
	unsigned 	i;

	for(i = 0; i < size; i++) r[i] = p[size-1-i];
	write_file(dir, name, r, reversible ? size : 0);
	free(r);
}

// a 6-bit VGA palette with the cars' colors, as BGR555
static void write_pal(const char * dir, const char * name, __BYTE__ * pal){

	FILE * 	fd;
	int 	n;

	for(n = 0; n < 4; n++) car_ramp(pal, n);
	fd = out(dir, name);
	for(n = 0; n < 0x100; n++){

		__WORD__ c = (pal[3*n] >> 1) | ((pal[3*n+1] >> 1) << 5) | ((pal[3*n+2] >> 1) << 10);
		fputc(c, fd); fputc(c >> 8, fd);
	}
	fclose(fd);
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
	race_car_shapes_t * 	shapes;
	FILE * 		fd;

	snprintf(bpa_name, sizeof(bpa_name), "TR%d.BPA", track);

	snprintf(entry, sizeof(entry), "TR%d-INF.BIN", track);
	bake_entry(bpa_name, entry, outdir, "inf.bin", 0);

	buf = calloc(1, wq*hq);
	snprintf(entry, sizeof(entry), "TR%d-VAI.BPK", track);
	src = read_entry(bpa_name, entry);
	bpk_decode4(wq*hq, 0x30a, buf, src);
	write_file(outdir, "vai.bin", buf, wq*hq);
	write_reversed(outdir, "rev.vai", buf, wq*hq);
	free(src);
	snprintf(entry, sizeof(entry), "TR%d-LR1.BPK", track);
	src = read_entry(bpa_name, entry);
	bpk_decode4(wq*hq, 0x30a, buf, src);
	write_file(outdir, "lr1.bin", buf, wq*hq);
	write_reversed(outdir, "rev.lr1", buf, wq*hq);
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

	// the race's pictures drawn over the track, one after the other
	{
		unsigned 	size = 0, o;

		for(i = 0; i < RACE_SPRITE_FILES; i++) size += race_sprite_files[i].size;
		buf = calloc(1, size);
		for(i = o = 0; i < RACE_SPRITE_FILES; o += race_sprite_files[i++].size){

			src = read_entry("ENGINE.BPA", race_sprite_files[i].name);
			bpk_decode2(buf+o, src);
			free(src);
		}
		write_file(outdir, "sprites.bin", buf, size);

		// the start lights' 3 frames and the finish flag's 64, the last two
		// files, as the tiles of the GBA's 64x64 OBJ and 32x64 one that
		// show them
		{
			const __BYTE__ * 	lamps = buf+size-0x4b000-0x3840;
			unsigned 		f, k, tx, ty, r;

			fd = out(outdir, "lamps.obj");
			for(f = 0; f < 67; f++) for(k = 0; k < 2; k++) for(ty = 0; ty < 8; ty++) for(tx = 0; tx < (k ? 4u : 8u); tx++) for(r = 0; r < 8; r++){

				unsigned 	x = 8*(tx+8*k), y = 8*ty+r;
				__BYTE__ 	row[8] = { 0 };

				if((y < 0x3c)&&(x < 0x50)) memcpy(row, lamps+0x12c0*f+0x50*y+x, 8);
				fwrite(row, 1, 8, fd);
			}
			fclose(fd);
		}
		free(buf);
	}

	// the HUD box's pictures
	{
		static const struct { const char * name; unsigned o; } hud[] = {
			{ "BIGNUM6.BPK", RACE_HUD_BIGNUM }, { "SMALFO4A.BPK", RACE_HUD_SMALFO },
			{ "SIDEBOM1.BPK", RACE_HUD_SIDEBOM }, { "SMALLBAR.BPK", RACE_HUD_SMALLBAR }
		};

		buf = calloc(1, RACE_HUD_SIZE);
		for(i = 0; i < 4; i++){

			src = read_entry("IBFILES.BPA", hud[i].name);
			bpk_decode4((i < 3 ? hud[i+1].o : RACE_HUD_SIZE)-hud[i].o, 0, buf+hud[i].o, src);
			free(src);
		}
		write_file(outdir, "hud.bin", buf, RACE_HUD_SIZE);
		free(buf);
	}

	bake_bpk2("MINES1A.BPK", outdir, "mines1a.bin", 0x140);
	bake_bpk2("BLOWI.BPK", outdir, "blowi.bin", 0x600);
	bake_bpk2("OBSTACLE.BPK", outdir, "obstacle.bin", 0x800);
	bake_bpk2("OBST_REV.BPK", outdir, "obst-rev.bin", 0x800);
	bake_entry("MENU.BPA", "CARCOL.PAL", outdir, "carcol.pal", 0x300);
	bake_entry("MENU.BPA", "COPPER.PAL", outdir, "copper.pal", 0x300);

	cars = calloc(13, CAR_FRAMES*CAR_W*CAR_W);
	for(v = 0; v < 13; v++){

		snprintf(entry, sizeof(entry), "%s%s", models[v%7], (v < 7) ? ".BPK" : "-S.BPK");
		src = read_entry("ENGINE.BPA", entry);
		bpk_decode2(cars+v*CAR_FRAMES*CAR_W*CAR_W, src);
		free(src);
	}
	write_file(outdir, "cars.img", cars, 13*CAR_FRAMES*CAR_W*CAR_W);
	shapes = calloc(13, sizeof(race_car_shapes_t));
	for(v = 0; v < 13; v++) race_car_shapes(cars+v*CAR_FRAMES*CAR_W*CAR_W, &shapes[v]);
	write_file(outdir, "cars.shp", shapes, 13*sizeof(race_car_shapes_t));

	// turned for a reversed track: race___4af3ch() reverses each car's 96
	// frames as a whole, then the order of the frames in each half
	{
		__POINTER__ 	rev = malloc(13*CAR_FRAMES*CAR_W*CAR_W);
		unsigned 	f, k, F = CAR_W*CAR_W;

		for(v = 0; v < 13; v++){
			for(f = 0; f < CAR_FRAMES; f++){

				const __BYTE__ * 	from = cars+(v*CAR_FRAMES+(f+CAR_FRAMES/2)%CAR_FRAMES)*F;
				__BYTE__ * 		to = rev+(v*CAR_FRAMES+f)*F;

				for(k = 0; k < F; k++) to[k] = from[F-1-k];
			}
		}
		write_file(outdir, "cars-rev.img", rev, 13*CAR_FRAMES*CAR_W*CAR_W);
		for(v = 0; v < 13; v++) race_car_shapes(rev+v*CAR_FRAMES*CAR_W*CAR_W, &shapes[v]);
		write_file(outdir, "cars-rev.shp", shapes, 13*sizeof(race_car_shapes_t));
		free(rev);
	}
	free(shapes);

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

static unsigned get16(const __BYTE__ * p){ return p[0]|(p[1] << 8); }
static unsigned get32(const __BYTE__ * p){ return p[0]|(p[1] << 8)|(p[2] << 16)|((unsigned)p[3] << 24); }
static void put16f(FILE * fd, unsigned v){ fputc(v&0xff, fd); fputc(v >> 8, fd); }
static void put32f(FILE * fd, unsigned v){ put16f(fd, v&0xffff); put16f(fd, v >> 16); }

// an 8-bit mono WAV, with a forward loop from loop to end if loop < end
static void write_wav(const char * dir, const char * name, const __BYTE__ * pcm, unsigned n, unsigned rate, unsigned loop){

	FILE * 	fd = out(dir, name);
	int 	looped = loop < n;

	fwrite("RIFF", 1, 4, fd); put32f(fd, 4+24+8+n+(n&1)+(looped ? 8+60 : 0)); fwrite("WAVE", 1, 4, fd);
	fwrite("fmt ", 1, 4, fd); put32f(fd, 16); put16f(fd, 1); put16f(fd, 1); put32f(fd, rate); put32f(fd, rate); put16f(fd, 1); put16f(fd, 8);
	fwrite("data", 1, 4, fd); put32f(fd, n); fwrite(pcm, 1, n, fd); if(n&1) fputc(0, fd);
	if(looped){

		// the sampler chunk: one loop, its end the last sample in it
		fwrite("smpl", 1, 4, fd); put32f(fd, 60);
		put32f(fd, 0); put32f(fd, 0); put32f(fd, 1000000000u/rate); put32f(fd, 60); put32f(fd, 0);
		put32f(fd, 0); put32f(fd, 0); put32f(fd, 1); put32f(fd, 0);
		put32f(fd, 0); put32f(fd, 0); put32f(fd, loop); put32f(fd, n-1); put32f(fd, 0); put32f(fd, 0);
	}
	fclose(fd);
}

// a CMF from MUSICS.BPA, decoded; its size in *size
static __POINTER__ read_cmf(const char * entry, unsigned * size){

	__POINTER__ 	p = read_entry_size("MUSICS.BPA", entry, size);

	dREncryption_decodeCMF(p, *size);

	return p;
}

// the music, and the effects as the desktop's mixer plays them (see
// sound2.c): each instrument's first sample, delta-coded, at
// samples_frequency*22050/0x10000 Hz
// an S3M's effects are 1-26 (A-Z); Maxmod jumps through a table with the
// number and goes astray past it. TR0-MUS.CMF has one stray 89, on a
// channel the song doesn't use, that the desktop's player passes over
static void s3m_clean(__POINTER__ p, unsigned size){

	unsigned 	n_ord = get16(p+0x20), n_ins = get16(p+0x22), n_pat = get16(p+0x24), i, o, row;

	for(i = 0; i < n_pat; i++){

		o = get16(p+0x60+n_ord+2*n_ins+2*i)*16;
		if(!o||(o+2 > size)) continue;
		o += 2;
		for(row = 0; (row < 64)&&(o < size); ){

			__BYTE__ 	b = p[o++];

			if(!b){ row++; continue; }
			if(b&0x20) o += 2;
			if(b&0x40) o += 1;
			if(b&0x80){

				if((o+1 < size)&&(p[o] > 26)) p[o] = p[o+1] = 0;
				o += 2;
			}
		}
	}
}

// an XM's instruments as sound effects, PREFIXnn.wav from 01 on, and
// their base volumes, 0-64, in VOLS_NAME
static void bake_xm(const char * cmf, const char * prefix, const char * vols_name, const char * outdir){

	unsigned 	size, o, i, k, n_ins, n_pat;
	__POINTER__ 	p;
	__BYTE__ 	vols[256];
	FILE * 		fd;

	p = read_cmf(cmf, &size);
	if(memcmp(p, "Extended Module: ", 17)){ fprintf(stderr, "%s isn't an XM\n", cmf); exit(1); }
	n_pat = get16(p+70);
	n_ins = get16(p+72);
	o = 60+get32(p+60);
	for(i = 0; i < n_pat; i++) o += get32(p+o)+get16(p+o+7);
	for(i = 0; i < n_ins; i++){

		const __BYTE__ * 	ins = p+o;
		unsigned 		n_smp = get16(ins+27), hsize = n_smp ? get32(ins+29) : 0, len = 0, loop = ~0u, rate = 8000;
		__BYTE__ * 		pcm = 0;
		char 			name[16];

		o += get32(ins);
		vols[i] = 0;
		if(n_smp){

			const __BYTE__ * 	h = p+o;
			const __BYTE__ * 	d = p+o+n_smp*hsize;
			unsigned 		bytes = get32(h), ls = get32(h+4), ll = get32(h+8), type = h[14], wide = (type&0x10) ? 2 : 1;
			int 			old = 0;

			len = bytes/wide;
			pcm = malloc(len ? len : 1);
			for(k = 0; k < len; k++){

				if(wide == 2){ old = (short)(old+(short)get16(d+2*k)); pcm[k] = ((old >> 8)&0xff)^0x80; }
				else { old = (signed char)(old+(signed char)d[k]); pcm[k] = (old&0xff)^0x80; }
			}
			if((type&3)&&ll){

				loop = ls/wide;
				if((ls+ll)/wide < len) len = (ls+ll)/wide;
			}
			vols[i] = h[12];
			rate = (unsigned)((unsigned long long)___71b70h_cdecl(XM_getPeriod((signed char)h[16]+0x31, (signed char)h[13]))*22050/0x10000);
			// past all the sample headers, then all their data
			for(k = 0, size = n_smp*hsize; k < n_smp; k++) size += get32(p+o+k*hsize);
			o += size;
		}
		if(!len){ free(pcm); pcm = calloc(1, 2); len = 2; }
		snprintf(name, sizeof(name), "%s%02u.wav", prefix, i+1);
		write_wav(outdir, name, pcm, len, rate, loop);
		free(pcm);
	}
	fd = out(outdir, vols_name);
	fwrite(vols, 1, n_ins, fd);
	fclose(fd);
	free(p);
	printf("%s: %u sound effects\n", cmf, n_ins);
}

// the menus' pictures, for the GBA's text screens: menu.pal as 256
// BGR555 colors, and pictures as 64x64 sprites of 8bpp 8x8 tiles, color 0
// see-through. The events' pictures, 104x128 and the loan shark's 96x96,
// are halved, each 2x2 block the menu color nearest its pixels' average,
// see-through if most are
static const __BYTE__ * 	menu_pal;

static int nearest_menu_color(int r, int g, int b){

	int 	n, best = 1, d, best_d = 1 << 30;

	for(n = 1; n < 0x100; n++){

		int 	dr = menu_pal[3*n]-r, dg = menu_pal[3*n+1]-g, db = menu_pal[3*n+2]-b;

		d = dr*dr+dg*dg+db*db;
		if(d < best_d){ best_d = d; best = n; }
	}

	return best;
}

static void put_obj64(FILE * fd, const __BYTE__ * px){

	__BYTE__ 	obj[64*64];
	int 		x, y;

	for(y = 0; y < 64; y++) for(x = 0; x < 64; x++) obj[64*((y/8)*8+x/8)+8*(y%8)+x%8] = px[64*y+x];
	fwrite(obj, 1, sizeof(obj), fd);
}

static void bake_menu_pictures(const char * outdir){

	static const struct { const char * name; int w, h; } pics[] = {
		{ "spons1b.bpk", 104, 128 },	// a streak, lapped, too slow
		{ "reaper_x.bpk", 104, 128 },	// the reaper
		{ "event_2.bpk", 104, 128 },	// the hit, not a scratch
		{ "drugdeal.bpk", 104, 128 },	// the steroids
		{ "dealer2b.bpk", 96, 96 },	// the loan shark
	};
	__BYTE__ 	pal[0x300], px[64*64];
	__POINTER__ 	src, img;
	char 		name[16];
	unsigned 	i, n, x, y, k;
	FILE * 		fd;

	src = read_entry("MENU.BPA", "menu.pal");
	memcpy(pal, src, sizeof(pal));
	free(src);
	menu_pal = pal;
	fd = out(outdir, "menu.pal");
	for(n = 0; n < 0x100; n++){

		__WORD__ c = (pal[3*n] >> 1) | ((pal[3*n+1] >> 1) << 5) | ((pal[3*n+2] >> 1) << 10);

		fputc(c, fd); fputc(c >> 8, fd);
	}
	fclose(fd);

	fd = out(outdir, "pictures.obj");
	for(i = 0; i < sizeof(pics)/sizeof(pics[0]); i++){

		src = read_entry("MENU.BPA", pics[i].name);
		img = calloc(1, pics[i].w*pics[i].h);
		bpk_decode2(img, src);
		memset(px, 0, sizeof(px));
		for(y = 0; y < (unsigned)pics[i].h/2; y++){
			for(x = 0; x < (unsigned)pics[i].w/2; x++){

				int 	r = 0, g = 0, b = 0, m = 0;

				for(k = 0; k < 4; k++){

					__BYTE__ 	c = img[pics[i].w*(2*y+k/2)+2*x+k%2];

					if(!c) continue;
					r += pal[3*c]; g += pal[3*c+1]; b += pal[3*c+2]; m++;
				}
				if(m >= 2) px[64*y+x] = nearest_menu_color(r/m, g/m, b/m);
			}
		}
		put_obj64(fd, px);
		free(img);
		free(src);
	}
	fclose(fd);

	// the drivers' faces as they are, 64x64, then facexx, nobody's
	fd = out(outdir, "faces.obj");
	for(i = 0; i <= 20; i++){

		if(i < 20) snprintf(name, sizeof(name), "face%02u.bpk", i+1);
		else strcpy(name, "facexx.bpk");
		src = read_entry("MENU.BPA", name);
		img = calloc(1, 64*64);
		bpk_decode2(img, src);
		put_obj64(fd, img);
		free(img);
		free(src);
	}
	fclose(fd);
}

static void bake_sound(int track, const char * outdir){

	char 		entry[32];
	unsigned 	size;
	__POINTER__ 	p;

	snprintf(entry, sizeof(entry), "TR%d-MUS.CMF", track);
	p = read_cmf(entry, &size);
	if(memcmp(p+0x2c, "SCRM", 4)){ fprintf(stderr, "%s isn't an S3M\n", entry); exit(1); }
	s3m_clean(p, size);
	snprintf(entry, sizeof(entry), "tr%d.s3m", track);
	write_file(outdir, entry, p, size);
	free(p);

	// the menus' music, as the track's
	p = read_cmf("MEN-MUS.CMF", &size);
	if(!memcmp(p+0x2c, "SCRM", 4)){

		s3m_clean(p, size);
		write_file(outdir, "men.s3m", p, size);
	}
	else if(!memcmp(p, "Extended Module: ", 17)) write_file(outdir, "men.xm", p, size);
	else { fprintf(stderr, "MEN-MUS.CMF is neither an S3M nor an XM\n"); exit(1); }
	printf("MEN-MUS.CMF: %s, %u bytes\n", memcmp(p+0x2c, "SCRM", 4) ? "XM" : "S3M", size);
	free(p);

	bake_xm("GEN-EFE.CMF", "efe", "sfx.bin", outdir);
	bake_xm("MEN-SAM.CMF", "men", "men.bin", outdir);
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
	write_pal(outdir, "track.pal", pal);
	{
		BPA * 		bpa = bpa_open(bpa_name);

		snprintf(entry, sizeof(entry), "TR%d-FLIP.PAL", track);
		bpa_search(bpa, entry);
		reversible = (bpa_entry_size(bpa) != 0);
		bpa_close(bpa);
		if(reversible){

			src = read_entry(bpa_name, entry);
			memcpy(pal, src, 0x300);
			free(src);
			write_pal(outdir, "track-rev.pal", pal);
		}
		else write_file(outdir, "track-rev.pal", pal, 0);
	}

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
	write_reversed(outdir, "rev.mas", mas, w*h);

	fd = out(outdir, "track.inf");
	put32(fd, tw); put32(fd, th); put32(fd, w); put32(fd, h);
	for(n = 0; n < 4; n++){ put32(fd, inf->startups[n].x); put32(fd, inf->startups[n].y); put32(fd, inf->startups[n].rot); }
	fclose(fd);

	bake_setup(track, w, h, outdir);
	bake_sound(track, outdir);
	bake_menu_pictures(outdir);
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
