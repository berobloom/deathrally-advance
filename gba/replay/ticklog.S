@ The race recorded by the desktop game's make ticklog, see race_replay.h.
@ The Makefile gives its path as TICKLOG_PATH.

	.section .rodata
	.align	4
	.global	ticklog
ticklog:
	.incbin	TICKLOG_PATH
