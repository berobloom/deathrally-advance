# A race on the GBA, run by the game's own race code: race_setup(),
# race_logic() and race_render() without its drawing, drawn with the GBA's
# tiles and sprites as in gba/proto. tools/gbabake bakes the tracks and the
# cars into the ROM. A quick race against three of the game's drivers, on
# the track chosen at the start, all ten of them in drrace.gba:
#
#   make -C gba/race
#
# TRACK=n builds drrace-trn.gba with only track n (0-9), which starts the
# race straight away.
#
# PRE=game/TICKPRE.BIN starts instead from the state the desktop game's
# menus left for a race on that track (make ticklog in the top directory,
# then race once), to check the GBA's setup against the desktop's.
#
# TICKS=60 runs a tick a frame, at 6/7 of the game's speed.
# RACE_CFLAGS="-DRACE_BENCH -DRACE_SEED=n" times a whole race with the AI
# driving every car, a tick a frame so that the race doesn't depend on the
# code's speed, and logs a hash of the race at its end.

GAME ?= ../../game
BUILD := build
BAKETOOL := ../../tools/gbabake/gbabake
ifdef TRACK
TRACKS := $(TRACK)
NAME := drrace-tr$(TRACK)
else
TRACKS := 0 1 2 3 4 5 6 7 8 9
NAME := drrace
endif
BAKES := $(TRACKS:%=$(BUILD)/bake-tr%)
empty :=
space := $(empty) $(empty)
comma := ,
# the files that are the same for every track come from the first
BAKE := $(firstword $(BAKES))

include ../game.mk

# DR_IMA_TILES: the track image the race draws into is the track's tiles
# in ROM, with the ones drawn into in EWRAM, see src/race/race_ima.c
CFLAGS += -DDR_RACE_NODRAW -DDR_IMA_TILES $(RACE_CFLAGS)
ifdef TICKS
CFLAGS += -DTICKS_PER_SEC=$(TICKS)
endif
ifdef PRE
ifndef TRACK
$(error PRE needs TRACK, the track the recording is on)
endif
CFLAGS += -DRACE_PRE
ASFLAGS_PRE := -DRACE_PRE
endif

ROM := $(NAME).gba
ELF := $(BUILD)/$(NAME).elf

all: $(ROM)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tDRRACE -cDRRC -m01

$(ELF): $(BUILD)/main.o $(BUILD)/assets-$(NAME).o $(LIBGCC_OBJS) $(GAME_LIB)
	$(CC) $(LDFLAGS) -Wl,-Map,$(BUILD)/drrace.map -o $@ $^ -lgba -lm

# rebuilt every time: PRE and RACE_CFLAGS change what goes in
$(BUILD)/main.o: main.c FORCE
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Wall -c -o $@ $<

$(BUILD)/assets-$(NAME).o: assets.S $(BAKES:%=%/.done) FORCE
ifdef PRE
	cp $(PRE) $(BAKE)/pre.bin
endif
	$(CC) $(ARCH) $(ASFLAGS_PRE) -DTRACKS="$(subst $(space),$(comma),$(strip $(TRACKS)))" -Wa,-I$(BAKE) -Wa,-I$(BUILD) -c -o $@ $<

$(BUILD)/bake-tr%/.done: $(BAKETOOL)
	mkdir -p $(@D)
	$(BAKETOOL) $(GAME) $* $(@D)
	touch $@

$(BAKETOOL): ../../tools/gbabake/gbabake.c ../../src/data/bpa.c ../../src/data/bpk.c ../../src/race/race_car_shapes.c
	$(MAKE) -C ../../tools/gbabake

.PHONY: all clean FORCE
clean:
	rm -rf $(BUILD) drrace*.gba
