# Replays a race recorded by the desktop game through race_logic() built for
# the GBA, times it, and checks it computes the same race. Needs devkitARM
# with libgba, and a recording:
#
#   make ticklog            # in the top directory, then race once
#   make -C gba/replay      # builds drreplay.gba from game/TICKLOG.BIN
#   make -C gba/replay TICKLOG=/path/to/TICKLOG.BIN

TICKLOG ?= ../../game/TICKLOG.BIN
BUILD := build

include ../game.mk

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

ROM := drreplay.gba
ELF := $(BUILD)/drreplay.elf

all: $(ROM)

$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(GBAFIX) $@ -tDRREPLAY -cDRRP -m01

$(ELF): $(BUILD)/main.o $(BUILD)/ticklog.o $(LIBGCC_OBJS) $(GAME_LIB)
	$(CC) $(LDFLAGS) -Wl,-Map,$(BUILD)/drreplay.map -o $@ $^ -lgba -lm

$(BUILD)/main.o: main.c FORCE
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/ticklog.o: ticklog.S $(TICKLOG)
	@mkdir -p $(@D)
	$(CC) $(ARCH) -DTICKLOG_PATH=\"$(abspath $(TICKLOG))\" -c -o $@ $<

# main.o follows FRAMES and PROFILE, so it's always rebuilt
FORCE:

.PHONY: all clean FORCE
clean:
	rm -rf $(BUILD) $(ROM)
