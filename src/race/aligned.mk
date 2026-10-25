# The race logic's files, which the GBA builds with DR_ALIGNED: their D(),
# W(), P() and Q() load whole words instead of bytes (see drally.h). Only
# files whose every such address is aligned may be here; drawing code
# writes 4 pixels at a time at any x. ALIGNED_CHECK=1 checks them, in the
# GBA builds and in the desktop game (make ALIGNED_CHECK=1).
RACE_ALIGNED_SRCS := race/race_logic.c race/race___4b62ch.c race/race___4c21ch.c race/race___4c434h.c \
	race/race___4dcach.c race/race___4ff50h.c race/race___514d8h.c race/race___5209ch.c \
	race/race___526ach.c race/race___52ac4h.c race/race___54668h.c race/race___55ae0h.c \
	race/race___56170h.c race/race___563ach.c
