// box.h
//
// A simple character based box and border drawing library.

#ifndef BOX_H
#define BOX_H

//#include "smalloc.h"

#define DATA_OFFSET 8 // offset from 8 bytes of x0y0,x1y1 coordds to actual data.

uint8_t *vbox_get(int16_t row1, int16_t col1, int16_t row2, int16_t col2);
void vbox_put(uint8_t *buf);
void box_draw(int16_t row1, int16_t col1, int16_t row2, int16_t col2, int line_type);
void box_erase(int16_t row1, int16_t col1, int16_t row2, int16_t col2);
void box_color(uint8_t row1, uint8_t col1, uint8_t row2, uint8_t col2);
void box_charfill(int16_t row1, int16_t col1, int16_t row2, int16_t col2, char c);
#endif //BOX_H
