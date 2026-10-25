# A race on the GBA, run by the game's own race code: race_logic() and
# race_render() without its drawing, drawn with the GBA's tiles and sprites
# as in gba/proto. The race starts from one the desktop game recorded:
#
#   make ticklog                         # in the top directory, then race once
#   make -C gba/race RACE=game/TICKLOG.BIN TRACK=n
#
# TRACK must be the recorded race's track; gbabake says if it isn't.

GAME ?= ../../game
TRACK ?= 8
RACE ?= ../../game/TICKLOG.BIN
BUILD := build
BAKE := $(BUILD)/bake-tr$(TRACK)
BAKETOOL := ../../tools/gbabake/gbabake

include ../game.mk

CFLAGS += -DDR_RACE_NODRAW $(RACE_CFLAGS)
ifdef TICKS
CFLAGS += -DTICKS_PER_SEC=$(TICKS)
endif

ROM := drrace-tr$(TRACK).gba
ELF := $(BUILD)/drrace-tr$(TRACK).elf

all: $(ROM)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tDRRACE -cDRRC -m01

$(ELF): $(BUILD)/main.o $(BUILD)/assets-tr$(TRACK).o $(LIBGCC_OBJS) $(GAME_LIB)
	$(CC) $(LDFLAGS) -Wl,-Map,$(BUILD)/drrace.map -o $@ $^ -lgba -lm

$(BUILD)/main.o: main.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Wall -c -o $@ $<

$(BUILD)/assets-tr$(TRACK).o: assets.S $(BAKE)/.done
	$(CC) $(ARCH) -Wa,-I$(BAKE) -c -o $@ $<

$(BAKE)/.done: $(BAKETOOL) $(RACE)
	mkdir -p $(BAKE)
	$(BAKETOOL) $(GAME) $(TRACK) $(BAKE) $(RACE)
	touch $@

$(BAKETOOL): ../../tools/gbabake/gbabake.c ../../src/data/bpa.c ../../src/data/bpk.c
	$(MAKE) -C ../../tools/gbabake

.PHONY: all clean
clean:
	rm -rf $(BUILD) drrace-tr*.gba
