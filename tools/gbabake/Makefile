CC := gcc
ROOT := ../..
CFLAGS := -O2 -I$(ROOT)/include -I/usr/include/SDL2

gbabake: gbabake.c $(ROOT)/src/data/bpa.c $(ROOT)/src/data/bpk.c $(ROOT)/src/race/race_car_shapes.c \
	$(ROOT)/src/data/drencryption.c $(ROOT)/src/sound/sound_xm.c $(ROOT)/src/core/watcom106.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	@rm -f gbabake
