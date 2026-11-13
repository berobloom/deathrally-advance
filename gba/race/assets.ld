/* gba_cart.ld's SECTIONS with this added at their end, by the Makefile
   (drrace.ld). The baked data (assets.S's .assets) after all the rest of the ROM: the
   code, its constants and the IWRAM and EWRAM images the start-up copies
   are in its first MBs, so that a cartridge smaller than the ROM still
   starts, and main.c's rom_whole() can say so. __assets_end is the ROM's
   end */
	.assets ALIGN(__pad_lma + SIZEOF(.pad), 16) : AT(ALIGN(__pad_lma + SIZEOF(.pad), 16))
	{
		KEEP(*(.assets))
		. = ALIGN(4);
		__assets_end = ABSOLUTE(.);
	}
