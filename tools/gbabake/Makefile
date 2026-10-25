CC := gcc
ROOT := ../..
CFLAGS := -O2 -I$(ROOT)/include -I/usr/include/SDL2

gbabake: gbabake.c $(ROOT)/src/data/bpa.c $(ROOT)/src/data/bpk.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	@rm -f gbabake
