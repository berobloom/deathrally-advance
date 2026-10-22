CC := gcc
MSBUILD := MSBuild.exe

DEFINES := -DDR_MULTIPLAYER -DIPXNET -DDR_LETTERBOX #-DDR_CDCHECK
INCLUDES := -Iinclude -I/usr/include/SDL2
FLAGS ?= -O3
FLAGS += -Werror -Wno-unused-result
LDFLAGS := -lm -lSDL2 -lSDL2_net

BUILD := build
TARGET := drally_linux

SRCS := $(wildcard src/*/*.c)
OBJS := $(SRCS:src/%.c=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(FLAGS) $(LDFLAGS)

$(BUILD)/%.o: src/%.c Makefile
	@mkdir -p $(@D)
	$(CC) -g -MMD -MP -c -o $@ $< $(INCLUDES) $(FLAGS) $(DEFINES)

-include $(DEPS)

.PHONY: all clean run check dRally.exe

run: $(TARGET)
	./scripts/drun

# LTO build: fails when files declare a function or global with different types
check:
	@$(MAKE) --no-print-directory BUILD=$(BUILD)/check TARGET=$(BUILD)/check/drally_lto \
		FLAGS="-O1 -flto=auto -Werror -Wno-unused-result"

dRally.exe:
	@$(MSBUILD)

clean:
	@rm -rf $(BUILD) $(TARGET)
