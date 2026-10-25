# The game's own code built for the GBA, shared by gba/replay and gba/race.
# A Makefile in gba/<dir> sets BUILD, includes this, and links $(GAME_LIB)
# and $(LIBGCC_OBJS) with $(LDFLAGS).

.DEFAULT_GOAL := all

DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
PREFIX := $(DEVKITARM)/bin/arm-none-eabi-
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
AR := $(PREFIX)ar
GBAFIX := $(DEVKITPRO)/tools/bin/gbafix

# the game's code without the SDL layer, in an archive so that only what
# is called gets linked
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

# libgcc's soft-float and division routines are ARM code; read from ROM
# over its 16-bit bus they take several times as long as from IWRAM
LIBGCC := $(shell $(CC) $(ARCH) -print-libgcc-file-name)
LIBGCC_IWRAM := _arm_addsubdf3 _arm_muldivdf3 _arm_cmpdf2 _arm_unorddf2 _arm_fixdfsi _arm_fixunsdfsi \
	_arm_truncdfsf2 _arm_addsubsf3 _arm_muldivsf3 _arm_cmpsf2 _arm_unordsf2 _arm_fixsfsi \
	_udivsi3 _divsi3 _dvmd_tls
LIBGCC_OBJS := $(LIBGCC_IWRAM:%=$(BUILD)/libgcc/%.o)

$(BUILD)/libgcc/%.o: $(LIBGCC)
	@mkdir -p $(@D)
	@cd $(@D) && $(AR) x $(LIBGCC) $*.o
	@$(OBJCOPY) --rename-section .text=.iwram $@

$(GAME_LIB): $(GAME_OBJS)
	@rm -f $@
	@$(AR) rcs $@ $^

$(BUILD)/game/%.o: ../../src/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c -o $@ $<
