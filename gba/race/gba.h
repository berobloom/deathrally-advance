// what gba/race's main.c and career_ui.c share
#include <gba_base.h>

// main.c's
void say(const char * fmt, ...);
u32 gba_cycles(void);
void gba_text_screen(void);
void gba_print(int col, int row, const char * s);
void gba_center(int row, const char * s);
int gba_text_width(const char * s, int n);
void gba_text_color(int bank, int shade);
void gba_text_clear(int y, int h);
int gba_big_width(const char * s);
void gba_print_big(int x, int y, const char * s);
void gba_cursor(int x, int y);
void gba_show_car(int model, int x, int y);
void gba_turn_car(void);
void gba_show_part(const u32 * frames, int n, int bounce, int x, int y);
void gba_car_color(int r, int g, int b);
void gba_picture(int slot, const u8 * obj, int x, int y);
u16 gba_frame(void);
void gba_career_reset(void);
void gba_career_race(int my_index, int num_cars);
// career_ui.c's
int gba_title(void);
void career_play(void);
