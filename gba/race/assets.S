@ Data baked by tools/gbabake from the game's files, see its list of them.
@ The Makefile puts the build directory and the first track's bake
@ directory on the include path, and lists the tracks in TRACKS.

	.macro	asset name, file
	.align	4
	.global	\name
\name:
	.incbin	"\file"
	.endm

#define RACE_GBA_CARS		7	// main.c's GBA_CAR_IMAGES
#define RACE_SHAPES_SIZE	8064	// sizeof(race_car_shapes_t), which main.c checks

@ the first n bytes of file: the GBA's cars are the 7 models, the
@ Adversary's last, without the spiked ones, which race the models' images
	.macro	asset_part name, file, n
	.align	4
	.global	\name
\name:
	.incbin	"\file", 0, \n
	.endm

	.section .rodata

@ the same for every track
	asset	setup_mines1a, mines1a.bin
	asset	setup_blowi, blowi.bin
	asset	setup_obstacle, obstacle.bin
	asset_part	cars_img, cars.img, RACE_GBA_CARS*96*40*40
	asset_part	cars_shp, cars.shp, RACE_GBA_CARS*RACE_SHAPES_SIZE
	asset_part	cars_rev_img, cars-rev.img, RACE_GBA_CARS*96*40*40
	asset_part	cars_rev_shp, cars-rev.shp, RACE_GBA_CARS*RACE_SHAPES_SIZE
	asset	setup_obst_rev, obst-rev.bin
	asset	sprites_bin, sprites.bin
	asset	hud_bin, hud.bin
	asset	lamps_obj, lamps.obj
	asset_part	cars_obj, cars-all.obj, RACE_GBA_CARS*4*96*5*5*64
	asset	carcol_pal, carcol.pal
	asset	copper_pal, copper.pal
	asset	menu_pal, menu.pal
	asset	pictures_obj, pictures.obj
	asset	faces_obj, faces.obj
	asset	sfx_bin, sfx.bin
	asset	men_bin, men.bin
	asset	soundbank_bin, soundbank.bin
#if defined(RACE_PRE)
	asset	race_pre, pre.bin
#endif

@ each track's
	.irp	n, TRACKS
	asset	tr\n\()_pal, bake-tr\n/track.pal
	asset	tr\n\()_inf, bake-tr\n/track.inf
	asset	tr\n\()_tiles, bake-tr\n/track.tiles
	asset	tr\n\()_mas, bake-tr\n/track.mas
	asset	tr\n\()_setup_inf, bake-tr\n/inf.bin
	asset	tr\n\()_vai, bake-tr\n/vai.bin
	asset	tr\n\()_lr1, bake-tr\n/lr1.bin
	asset	tr\n\()_ski, bake-tr\n/ski.tab
	asset	tr\n\()_blo, bake-tr\n/blo.tab
	asset	tr\n\()_lit, bake-tr\n/lit.tab
	asset	tr\n\()_varjo, bake-tr\n/varjo.tab
	asset	tr\n\()_drv, bake-tr\n/drv.dat
	asset	tr\n\()_ohi, bake-tr\n/ohi.dat
	@ reversed, or empty if the game doesn't reverse it
	asset	tr\n\()_rev_pal, bake-tr\n/track-rev.pal
tr\n\()_rev_pal_end:
	asset	tr\n\()_rev_mas, bake-tr\n/rev.mas
	asset	tr\n\()_rev_vai, bake-tr\n/rev.vai
	asset	tr\n\()_rev_lr1, bake-tr\n/rev.lr1
	.endr

@ race_track_t race_tracks[race_tracks_n]: the track's number, then its files
	.align	2
	.global	race_tracks
race_tracks:
	.irp	n, TRACKS
	.word	\n, tr\n\()_pal, tr\n\()_inf, tr\n\()_tiles, tr\n\()_mas, tr\n\()_setup_inf, tr\n\()_vai, tr\n\()_lr1
	.word	tr\n\()_ski, tr\n\()_blo, tr\n\()_lit, tr\n\()_varjo, tr\n\()_drv, tr\n\()_ohi
	.word	tr\n\()_rev_pal_end-tr\n\()_rev_pal, tr\n\()_rev_pal, tr\n\()_rev_mas, tr\n\()_rev_vai, tr\n\()_rev_lr1
	.endr
	.global	race_tracks_n
race_tracks_n:
	.word	(race_tracks_n-race_tracks)/(19*4)
