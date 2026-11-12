# gbarun: runs a GBA ROM headless in libmgba, see gbarun.c
# needs mGBA's library and headers (Debian/Ubuntu: libmgba-dev) and zlib

CC := gcc
CFLAGS := -O2 -g -Wall -Wextra

gbarun: gbarun.c
	$(CC) $(CFLAGS) -o $@ $< -lmgba -lz

clean:
	rm -f gbarun
