@ Data baked by tools/gbabake from the game's files, see its list of them.
@ The Makefile puts the bake directory on the include path.

	.macro	asset name, file
	.align	4
	.global	\name
\name:
	.incbin	"\file"
	.endm

	.section .rodata

	asset	track_pal, track.pal
	asset	track_inf, track.inf
	asset	track_tiles, track.tiles
	asset	track_mas, track.mas
	asset	setup_inf, inf.bin
	asset	setup_vai, vai.bin
	asset	setup_lr1, lr1.bin
	asset	setup_ski, ski.tab
	asset	setup_blo, blo.tab
	asset	setup_lit, lit.tab
	asset	setup_varjo, varjo.tab
	asset	setup_drv, drv.dat
	asset	setup_ohi, ohi.dat
	asset	setup_mines1a, mines1a.bin
	asset	setup_blowi, blowi.bin
	asset	setup_obstacle, obstacle.bin
	asset	cars_img, cars.img
	asset	cars_obj, cars-all.obj
#if defined(RACE_PRE)
	asset	race_pre, pre.bin
#endif
