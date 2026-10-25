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
	-DDR_REPLAY -DDR_INLINE_MATH -I../../include/nosdl -I../../include -I../../src/race -I$(DEVKITPRO)/libgba/include
# long calls: the soft-float routines in IWRAM are then called through a
# bx in the caller instead of a veneer run as ARM code from ROM
CFLAGS += -mlong-calls
CFLAGS += -D'DR_HOT=__attribute__((section(".iwram"),target("arm"),noinline))' \
	-D'DR_COLD=__attribute__((section(".sbss")))'
LDFLAGS := -specs=gba.specs $(ARCH) -Wl,--gc-sections -L$(DEVKITPRO)/libgba/lib

ifneq ($(FRAMES),)
CFLAGS += -DREPLAY_FRAMES=$(FRAMES)
endif
# PROFILE=1 times each part of race_logic(), PROFILE=2 also counts the
# soft-float calls (and makes everything slower), see main.c
ifneq ($(PROFILE),)
CFLAGS += -DPROFILE=$(PROFILE)
LDFLAGS += $(foreach f,race___4b62ch race___4c434h race___4dcach race___514d8h race___52ac4h \
	race___526ach race___4ff50h race___54668h race___55ae0h race___56170h race___563ach \
	race___5209ch race___4c21ch,-Wl,--wrap=$(f))
ifeq ($(PROFILE),2)
LDFLAGS += $(foreach f,dadd dsub dmul ddiv fadd fsub fmul fdiv f2d d2f i2d ui2d d2iz d2uiz f2iz i2f \
	dcmplt dcmpgt dcmpeq fcmpgt fcmpge fcmple fcmplt,-Wl,--wrap=__aeabi_$(f))
endif
endif

# libgcc's soft-float and division routines are ARM code; read from ROM
# over its 16-bit bus they take several times as long as from IWRAM
LIBGCC := $(shell $(CC) $(ARCH) -print-libgcc-file-name)
LIBGCC_IWRAM := _arm_addsubdf3 _arm_muldivdf3 _arm_cmpdf2 _arm_unorddf2 _arm_fixdfsi _arm_fixunsdfsi \
	_arm_truncdfsf2 _arm_addsubsf3 _arm_muldivsf3 _arm_cmpsf2 _arm_unordsf2 _arm_fixsfsi \
	_udivsi3 _divsi3 _dvmd_tls
LIBGCC_OBJS := $(LIBGCC_IWRAM:%=$(BUILD)/libgcc/%.o)

ROM := drreplay.gba
ELF := $(BUILD)/drreplay.elf

all: $(ROM)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tDRREPLAY -cDRRP -m01

$(ELF): $(BUILD)/main.o $(BUILD)/ticklog.o $(LIBGCC_OBJS) $(GAME_LIB)
	$(CC) $(LDFLAGS) -Wl,-Map,$(BUILD)/drreplay.map -o $@ $^ -lgba -lm

$(BUILD)/libgcc/%.o: $(LIBGCC)
	@mkdir -p $(@D)
	@cd $(@D) && $(AR) x $(LIBGCC) $*.o
	@$(OBJCOPY) --rename-section .text=.iwram $@

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
	$(CC) $(ARCH) -DTICKLOG_PATH=\"$(abspath $(TICKLOG))\" -c -o $@ $<

# main.o follows FRAMES and PROFILE, so it's always rebuilt
FORCE:

.PHONY: all clean FORCE
clean:
	rm -rf $(BUILD) $(ROM)
