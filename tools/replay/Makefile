# replay: runs a recorded race through race_logic() on the desktop, see replay.c

CC := gcc
# IMA_TILES=1 builds replay-tiles, with the track image as the GBA keeps
# it (DR_IMA_TILES), to check that against the plain one with --race
ifdef IMA_TILES
BUILD := build-tiles
TARGET := replay-tiles
EXTRA := -DDR_IMA_TILES -DRACE_IMA_TILES=$(IMA_TILES)
else
BUILD := build
TARGET := replay
endif

# the game's code without the SDL layer, built as gba/replay builds it, in
# an archive so that only what race_logic() needs is linked
SDL_SRCS := core/drally.c core/drally_linux_c.c core/events.c core/keyboard.c core/system_linux_c.c \
	gfx/cinem.c sound/sound.c sound/sound_api.c
GAME_SRCS := $(filter-out $(SDL_SRCS),$(patsubst ../../src/%,%,$(wildcard ../../src/*/*.c)))
GAME_OBJS := $(GAME_SRCS:%.c=$(BUILD)/game/%.o)
GAME_LIB := $(BUILD)/libgame.a

CFLAGS := -O2 -g -ffp-contract=off -DDR_RACE_NODRAW -DDR_CAREER_NODRAW -ffunction-sections -fdata-sections -Wno-unused-result -DDR_REPLAY -DDR_INLINE_MATH -I../../include/nosdl -I../../include -I../../src/race $(EXTRA)

$(TARGET): $(BUILD)/replay.o $(GAME_LIB)
	$(CC) -Wl,--gc-sections -o $@ $^ -lm

$(BUILD)/replay.o: replay.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/game/%.o: ../../src/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

$(GAME_LIB): $(GAME_OBJS)
	@rm -f $@
	@ar rcs $@ $^

-include $(GAME_OBJS:.o=.d)

.PHONY: clean
clean:
	rm -rf build build-tiles replay replay-tiles
