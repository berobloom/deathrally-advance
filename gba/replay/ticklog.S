@ The race recorded by the desktop game's make ticklog, see race_replay.h.
@ The Makefile puts its directory on the include path.

	.section .rodata
	.align	4
	.global	ticklog
ticklog:
	.incbin	"TICKLOG.BIN"
