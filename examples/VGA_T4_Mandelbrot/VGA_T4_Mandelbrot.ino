// Mandelbrot example from uVGA example.

#include "VGA_4bit_T4.h"

const byte cmap[] =
{
	0b00000000, 0b11100000, 0b11100100, 0b11101000, 0b11101100, 0b11110000, 0b11110100, 0b11111000, 0b11111100,
	0b11011100, 0b10111100, 0b10011100, 0b01111100, 0b01011100, 0b00111100, 0b00011100, 0b00011101, 0b00011110,
	0b00011111, 0b00011011, 0b00010111, 0b00010011, 0b00001111, 0b00001011, 0b00000111, 0b00000011, 0b00100011,
	0b01000011, 0b01100011, 0b10000011, 0b10100011, 0b11000011, 0b11100011, 0b11100010, 0b11100001, 0b11100000,
	0b00000000
};

uint16_t fbWidth, fbHeight;

// Uncomment one of the following screen resolutions. Try them all:)
//const vga_timing *timing = &t1024x768x60;
//const vga_timing *timing = &t800x600x60;
//const vga_timing *timing = &t640x480x60;
const vga_timing *timing = &t640x400x70;

// Must use this instance name. It's used in the driver.
FlexIO2VGA vga4bit;

void setup() {
  Serial.begin(9600);
  vga4bit.stop();
  // Setup VGA display: 800x600x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits
  vga4bit.begin(*timing, false, false, 4);
  // Get display dimensions
  vga4bit.getFbSize(&fbWidth, &fbHeight);
  // Clear graphic screen
  vga4bit.clear(VGA_BLUE);
}


void loop() {
  int max = 256;
  int row, col;
  float c_re, c_im, x, y, x_new;
  int iteration;


  for (row = 0; row < fbHeight; row++) {
    for (col = 0; col < fbWidth; col++) {
      c_re = (col - fbWidth / 1.5) * 4.0 / fbWidth;
      c_im = (row - fbHeight / 2.0) * 4.0 / fbWidth;
      x = 0;
      y = 0;
      iteration = 0;
      while (((x * x + y * y) <= 4) && (iteration < max)) {
        x_new = x * x - y * y + c_re;
        y = 2 * x * y + c_im;
        x = x_new;
        iteration++;
      }
      if (iteration < max)
        vga4bit.drawPixel(col, row, cmap[iteration]);
      else
        vga4bit.drawPixel(col, row, 0);
        vga4bit.fbUpdate(false);
    }
  }
  for(;;);
}
