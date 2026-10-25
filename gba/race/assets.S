@ Data baked by tools/gbabake from the game's TRn.BPA and a recorded race.
@ The Makefile puts the bake directory on the include path.

	.section .rodata
	.align	4

	.global	track_pal
track_pal:
	.incbin	"track.pal"

	.align	4
	.global	track_inf
track_inf:
	.incbin	"track.inf"

	.align	4
	.global	track_tiles
track_tiles:
	.incbin	"track.tiles"

	.align	4
	.global	track_mas
track_mas:
	.incbin	"track.mas"

	.align	4
	.global	cars_obj
cars_obj:
	.incbin	"cars.obj"

	.align	4
	.global	race_bin
race_bin:
	.incbin	"race.bin"
