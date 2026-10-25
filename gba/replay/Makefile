# Replays a race recorded by the desktop game through race_logic() built for
# the GBA, times it, and checks it computes the same race. Needs devkitARM
# with libgba, and a recording:
#
#   make ticklog            # in the top directory, then race once
#   make -C gba/replay      # builds drreplay.gba from game/TICKLOG.BIN
#   make -C gba/replay TICKLOG=/path/to/TICKLOG.BIN

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
PREFIX := $(DEVKITARM)/bin/arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
AR := $(PREFIX)ar
GBAFIX := $(DEVKITPRO)/tools/bin/gbafix

TICKLOG ?= ../../game/TICKLOG.BIN
BUILD := build

# the game's code without the SDL layer, in an archive so that only what
# race_logic() needs is linked
SDL_SRCS := core/drally.c core/drally_linux_c.c core/events.c core/keyboard.c core/system_linux_c.c \
	gfx/cinem.c sound/sound.c sound/sound_api.c
GAME_SRCS := $(filter-out $(SDL_SRCS),$(patsubst ../../src/%,%,$(wildcard ../../src/*/*.c)))
GAME_OBJS := $(GAME_SRCS:%.c=$(BUILD)/game/%.o)
GAME_LIB := $(BUILD)/libgame.a

ARCH := -mthumb -mthumb-interwork -mcpu=arm7tdmi -mtune=arm7tdmi
CFLAGS := $(ARCH) -O2 -ffunction-sections -fdata-sections -Wno-unused-result \
	-DDR_REPLAY -I../include -I../../include -I../../src/race -I$(DEVKITPRO)/libgba/include
LDFLAGS := -specs=gba.specs $(ARCH) -Wl,--gc-sections -L$(DEVKITPRO)/libgba/lib

ifneq ($(FRAMES),)
CFLAGS += -DREPLAY_FRAMES=$(FRAMES)
endif
# PROFILE=1 times each part of race_logic(), see main.c
ifneq ($(PROFILE),)
CFLAGS += -DPROFILE
LDFLAGS += $(foreach f,race___4b62ch race___4c434h race___4dcach race___514d8h race___52ac4h \
	race___526ach race___4ff50h race___54668h race___55ae0h race___56170h race___563ach \
	race___5209ch race___4c21ch sin cos,-Wl,--wrap=$(f)) \
	$(foreach f,dadd dsub dmul ddiv fadd fsub fmul fdiv f2d d2f i2d ui2d d2iz d2uiz f2iz i2f \
	dcmplt dcmpgt dcmpeq fcmpgt fcmpge fcmple fcmplt,-Wl,--wrap=__aeabi_$(f))
endif

ROM := drreplay.gba
ELF := $(BUILD)/drreplay.elf

all: $(ROM)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tDRREPLAY -cDRRP -m01

$(ELF): $(BUILD)/main.o $(BUILD)/ticklog.o $(GAME_LIB)
	$(CC) $(LDFLAGS) -Wl,-Map,$(BUILD)/drreplay.map -o $@ $^ -lgba -lm

$(GAME_LIB): $(GAME_OBJS)
	@rm -f $@
	@$(AR) rcs $@ $^

$(BUILD)/main.o: main.c FORCE
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/game/%.o: ../../src/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/ticklog.o: ticklog.S $(TICKLOG)
	@mkdir -p $(@D)
	$(CC) $(ARCH) -Wa,-I$(dir $(TICKLOG)) -c -o $@ $<

# main.o follows FRAMES and PROFILE, so it's always rebuilt
FORCE:

.PHONY: all clean FORCE
clean:
	rm -rf $(BUILD) $(ROM)
