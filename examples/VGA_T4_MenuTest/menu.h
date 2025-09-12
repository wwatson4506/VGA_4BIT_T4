#ifndef MENU_DEFINED
#define MENU_DEFINED

#include "USBHost_t36.h"
#include "VGA_4bit_T4.h"
#include "USBKeyboard.h"
#include "USBmouseVGA.h"
#include "getKeyMouse.h"

#define B_PATTERN 178

uint8_t *menu_bar(int16_t row, int16_t col, const char *string, int *choice);
uint8_t *menu_drop(int row, int col, const char **strary, int *choice);
uint8_t *menu_message(int row, int col, const char **strary);
void menu_erase(uint8_t *buf);

void menu_box_lines(int line_type);
void menu_shadow(int on_off);
void menu_line_color(int lines);
void menu_hiletter(int hiletter);
void menu_text_color(int text);
void menu_hitext(int hitext);
void menu_back(int back);
void menu_title(int title);
void menu_prompt(int prompt);
void menu_hilight_back(int hiback);

#endif
