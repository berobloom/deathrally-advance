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
# DR_MAS_BLOCKS: the track's mask is gbabake's track.mbk, in 8x8 blocks of
# 4 bits, forward and reversed, see src/race/race_mas.c
# DR_CAREER_NODRAW: the career's menu functions without their drawing, see
# drally_career.h; career_ui.c draws its own screens
CFLAGS += -DDR_RACE_NODRAW -DDR_IMA_TILES -DDR_MAS_BLOCKS -DDR_CAREER_NODRAW $(RACE_CFLAGS)
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

# Maxmod's module player runs once a tick of the song: from ROM, with its
# state in EWRAM, it leaves IWRAM to the mixer, which runs for every sample
LIBMM := $(DEVKITPRO)/libgba/lib/libmm.a
LIBMM_ROM := mm_mas mm_mas_arm mm_effect
$(BUILD)/libmm/%.o: $(LIBMM)
	@mkdir -p $(@D)
	@cd $(@D) && $(AR) x $(LIBMM) $*.o
	@$(OBJCOPY) --rename-section .iwram=.text --rename-section .bss=.sbss $@

# the assets last in the ROM: gba.specs with gba_cart.ld as drrace.ld,
# which has assets.ld's section after the rest
$(BUILD)/drrace.ld: assets.ld
	@mkdir -p $(@D)
	sed '/^\s*__rom_end__ = /r assets.ld' $(DEVKITARM)/arm-none-eabi/lib/gba_cart.ld > $@

$(BUILD)/drrace.specs: $(BUILD)/drrace.ld
	sed 's|-T gba_cart.ld%s|-T $(CURDIR)/$(BUILD)/drrace.ld|' $(DEVKITARM)/arm-none-eabi/lib/gba.specs > $@

$(ELF): $(BUILD)/main.o $(BUILD)/career_ui.o $(BUILD)/sound.o $(BUILD)/assets-$(NAME).o $(LIBGCC_OBJS) $(LIBMM_ROM:%=$(BUILD)/libmm/%.o) $(GAME_LIB) $(BUILD)/drrace.specs
	$(CC) $(subst -specs=gba.specs,-specs=$(BUILD)/drrace.specs,$(LDFLAGS)) -Wl,-Map,$(BUILD)/drrace.map -o $@ $(filter-out %.specs,$^) -lmm -lgba -lm

# rebuilt every time: PRE and RACE_CFLAGS change what goes in
$(BUILD)/main.o: main.c gba.h FORCE
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Wall -c -o $@ $<

$(BUILD)/career_ui.o: career_ui.c gba.h FORCE
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -Wall -c -o $@ $<

# the music of each track and the effects, for Maxmod: the race's effects
# first, so that their ids are 0 on, the menus' after them, the end's
# film's and the Adversary's, then the tracks' modules in TRACKS' order and the menus' last
MMUTIL := $(DEVKITPRO)/tools/bin/mmutil
$(BUILD)/soundbank-$(NAME).bin: $(BAKES:%=%/.done)
	$(MMUTIL) -o$@ -h$(BUILD)/soundbank-$(NAME).h $(sort $(wildcard $(BAKE)/efe*.wav)) $(sort $(wildcard $(BAKE)/men*.wav)) $(sort $(wildcard $(BAKE)/end*.wav)) $(sort $(wildcard $(BAKE)/adv*.wav)) $(foreach t,$(TRACKS),$(BUILD)/bake-tr$(t)/tr$(t).s3m) $(BAKE)/men.s3m > /dev/null

$(BUILD)/sound.o: sound.c $(BUILD)/soundbank-$(NAME).bin
	$(CC) $(CFLAGS) -I$(BUILD) -include $(BUILD)/soundbank-$(NAME).h -Wall -c -o $@ $<

$(BUILD)/assets-$(NAME).o: assets.S $(BAKES:%=%/.done) $(BUILD)/soundbank-$(NAME).bin FORCE
ifdef PRE
	cp $(PRE) $(BAKE)/pre.bin
endif
	# assets.S's soundbank.bin: this ROM's, whichever was built last
	cp $(BUILD)/soundbank-$(NAME).bin $(BUILD)/soundbank.bin
	$(CC) $(ARCH) $(ASFLAGS_PRE) -DTRACKS="$(subst $(space),$(comma),$(strip $(TRACKS)))" -Wa,-I$(BAKE) -Wa,-I$(BUILD) -c -o $@ $<

$(BUILD)/bake-tr%/.done: $(BAKETOOL)
	mkdir -p $(@D)
	$(BAKETOOL) $(GAME) $* $(@D)
	touch $@

$(BAKETOOL): ../../tools/gbabake/gbabake.c ../../src/data/bpa.c ../../src/data/bpk.c ../../src/race/race_car_shapes.c \
		../../src/data/car_data.c ../../src/menu/menu_data.c
	$(MAKE) -C ../../tools/gbabake

.PHONY: all clean FORCE
clean:
	rm -rf $(BUILD) drrace*.gba
