# Death Rally GBA prototype. Needs devkitARM with libgba, and the game data
# in ../../game (or GAME=...). TRACK picks the track, 0-9.
#
#   make                 # builds drproto-tr0.gba
#   make TRACK=5         # builds drproto-tr5.gba

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
PREFIX := $(DEVKITARM)/bin/arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
GBAFIX := $(DEVKITPRO)/tools/bin/gbafix

GAME ?= ../../game
TRACK ?= 0
BUILD := build
BAKE := $(BUILD)/bake-tr$(TRACK)
BAKETOOL := ../../tools/gbabake/gbabake

ARCH := -mthumb -mthumb-interwork -mcpu=arm7tdmi -mtune=arm7tdmi
CFLAGS := $(ARCH) -O2 -Wall -I$(DEVKITPRO)/libgba/include
LDFLAGS := -specs=gba.specs $(ARCH) -L$(DEVKITPRO)/libgba/lib

ROM := drproto-tr$(TRACK).gba
ELF := $(BUILD)/drproto-tr$(TRACK).elf

all: $(ROM)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tDRPROTO -cDRPR -m01

$(ELF): $(BUILD)/main.o $(BUILD)/assets-tr$(TRACK).o
	$(CC) $(LDFLAGS) -o $@ $^ -lgba

$(BUILD)/main.o: main.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/assets-tr$(TRACK).o: assets.S $(BAKE)/.done
	$(CC) $(ARCH) -Wa,-I$(BAKE) -c -o $@ $<

$(BAKE)/.done: $(BAKETOOL)
	mkdir -p $(BAKE)
	$(BAKETOOL) $(GAME) $(TRACK) $(BAKE)
	touch $@

$(BAKETOOL): ../../tools/gbabake/gbabake.c ../../src/data/bpa.c ../../src/data/bpk.c
	$(MAKE) -C ../../tools/gbabake

$(BUILD):
	mkdir -p $@

.PHONY: all clean
clean:
	rm -rf $(BUILD) drproto-tr*.gba
