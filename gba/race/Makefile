# A race on the GBA, run by the game's own race code: race_setup(),
# race_logic() and race_render() without its drawing, drawn with the GBA's
# tiles and sprites as in gba/proto. tools/gbabake bakes the track and the
# cars into the ROM. A quick race on track n (0-9) against three of the
# game's drivers:
#
#   make -C gba/race TRACK=n
#
# PRE=game/TICKPRE.BIN starts instead from the state the desktop game's
# menus left for a race on that track (make ticklog in the top directory,
# then race once), to check the GBA's setup against the desktop's.

GAME ?= ../../game
TRACK ?= 8
BUILD := build
BAKE := $(BUILD)/bake-tr$(TRACK)
BAKETOOL := ../../tools/gbabake/gbabake

include ../game.mk

CFLAGS += -DDR_RACE_NODRAW -DTRACK=$(TRACK) $(RACE_CFLAGS)
ifdef TICKS
CFLAGS += -DTICKS_PER_SEC=$(TICKS)
endif
ifdef PRE
CFLAGS += -DRACE_PRE
ASFLAGS_PRE := -DRACE_PRE
endif

ROM := drrace-tr$(TRACK).gba
ELF := $(BUILD)/drrace-tr$(TRACK).elf

all: $(ROM)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tDRRACE -cDRRC -m01

$(ELF): $(BUILD)/main.o $(BUILD)/assets-tr$(TRACK).o $(LIBGCC_OBJS) $(GAME_LIB)
	$(CC) $(LDFLAGS) -Wl,-Map,$(BUILD)/drrace.map -o $@ $^ -lgba -lm

# rebuilt every time: PRE and RACE_CFLAGS change what goes in
$(BUILD)/main.o: main.c FORCE
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Wall -c -o $@ $<

$(BUILD)/assets-tr$(TRACK).o: assets.S $(BAKE)/.done FORCE
ifdef PRE
	cp $(PRE) $(BAKE)/pre.bin
endif
	$(CC) $(ARCH) $(ASFLAGS_PRE) -Wa,-I$(BAKE) -c -o $@ $<

$(BAKE)/.done: $(BAKETOOL)
	mkdir -p $(BAKE)
	$(BAKETOOL) $(GAME) $(TRACK) $(BAKE)
	touch $@

$(BAKETOOL): ../../tools/gbabake/gbabake.c ../../src/data/bpa.c ../../src/data/bpk.c
	$(MAKE) -C ../../tools/gbabake

.PHONY: all clean FORCE
clean:
	rm -rf $(BUILD) drrace-tr*.gba
