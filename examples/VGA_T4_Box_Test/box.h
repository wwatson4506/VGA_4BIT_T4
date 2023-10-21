// box.h
#ifndef BOX_H
#define BOX_H

uint8_t *vbox_get(int16_t row1, int16_t col1, int16_t row2, int16_t col2);
void vbox_put(uint8_t *buf);
void box_draw(int16_t row1, int16_t col1, int16_t row2, int16_t col2, int line_type);
void box_erase(int16_t row1, int16_t col1, int16_t row2, int16_t col2);
void box_color(int16_t row1, int16_t col1, int16_t row2, int16_t col2);
void box_charfill(int16_t row1, int16_t col1, int16_t row2, int16_t col2, char c);

#endif //BOX_H
