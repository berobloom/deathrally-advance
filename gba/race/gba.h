// what gba/race's main.c and career_ui.c share
#include <gba_base.h>

// main.c's
void say(const char * fmt, ...);
u32 gba_cycles(void);
void gba_text_screen(void);
void gba_print(int col, int row, const char * s);
void gba_show_car(int model, int x, int y);
void gba_turn_car(void);
u16 gba_frame(void);
void gba_career_reset(void);
void gba_career_race(int my_index, int num_cars);
// career_ui.c's
int gba_title(void);
void career_play(void);
