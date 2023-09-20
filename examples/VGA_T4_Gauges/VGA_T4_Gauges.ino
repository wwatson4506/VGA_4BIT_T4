/*
 VGA_T4_Gauges.ino
 Taken from Sumotoy's RA8875 library and adapted for
 4 bit VGA driver on Teensy 4.1.
*/
#include "VGA_4bit_T4.h"
#include <math.h>

#define FONTSIZE 16

//const vga_timing *timing = &t1024x768x60;
const vga_timing *timing = &t800x600x60;
//const vga_timing *timing = &t640x480x60;
//const vga_timing *timing = &t640x400x70;

// Must use this instance name (vga4bit). It is used in the driver.
FlexIO2VGA vga4bit;
static int fb_width, fb_height;

// Array of vga4bit Basic Colors
const uint16_t myColors[] = {
  VGA_BLACK,  // 0
  VGA_BLUE,   // 1
  VGA_GREEN,  // 2
  VGA_CYAN,   // 3
  VGA_RED,    // 4
  VGA_MAGENTA,// 5
  VGA_YELLOW, // 6
  VGA_WHITE,  // 7
  VGA_GREY,   // 8
  VGA_BRIGHT_BLUE,  // 9
  VGA_BRIGHT_GREEN, // 10
  VGA_BRIGHT_CYAN,  // 11
  VGA_BRIGHT_RED,   // 12
  VGA_BRIGHT_MAGENTA, // 13
  VGA_BRIGHT_YELLOW,  // 14
  VGA_BRIGHT_WHITE  // 15
};

volatile int16_t curVal[6] = { -1, -1, -1, -1, -1, -1};
volatile int16_t oldVal[6] = {0, 0, 0, 0, 0, 0};
const int16_t posx[6] = {63, 193, 323, 453, 583, 713};
const int16_t posy[6] = {63, 63, 63, 63, 63, 63};
const int16_t radius[6] = {63, 63, 63, 63, 63, 63};
const uint8_t analogIn[6] = {A0, A1, A2, A3, A8, A9};
const uint16_t needleColors[6] = {myColors[2], myColors[11], myColors[13], myColors[14], myColors[15], myColors[12]};
const uint8_t degreesVal[6][2] = {
  {150, 240},
  {150, 240},
  {150, 240},
  {150, 240},
  {150, 240},
  {150, 240},
};

void setup() {
  Serial.begin(9600);
  vga4bit.stop();
  // Setup VGA display: 800x600x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits
  vga4bit.begin(*timing, false, false, 4);
  // Get display dimensions
  vga4bit.getFbSize(&fb_width, &fb_height);
  // Set fontsize 8x16 or (8x8 available)
  vga4bit.setFontSize(FONTSIZE);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLUE); 
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  // Clear screen to background color
  vga4bit.clear(VGA_BLUE);
  // Move text cursor to last line of display
  vga4bit.textxy(0,fb_height-FONTSIZE);
  vga4bit.printf("4 bit VGA version of Sumotoy's gauges example...");
  // Draw guage face (6 gauges)
  for (uint8_t i = 0; i < 6; i++) {
    drawGauge(posx[i], posy[i], radius[i]);
  }
}

void loop() {
  // Display random needle values (0 t0 254) for 6 gauges repeatedly
  for (uint8_t i = 0; i < 6; i++) {
    curVal[i] = random(254);
    if (curVal[i] == 0) Serial.println("found a zero");
    drawNeedle(i, myColors[1]);
  }
}

void drawGauge(uint16_t x, uint16_t y, uint16_t r) {
  vga4bit.drawCircle(x, y, r, myColors[1]); //draw instrument container
  faceHelper(x, y, r, 150, 390, 1.3); //draw major ticks
  if (r > 15) faceHelper(x, y, r, 165, 375, 1.1); //draw minor ticks
}

void faceHelper(uint16_t x, uint16_t y, uint16_t r, int from, int to, float dev) {
  float dsec, rdev;
  uint16_t w, h, nx, ny;
  int i;
  rdev = r / dev;
  for (i = from; i <= to; i += 30) {
    dsec = i * (PI / 180.0);
    nx = (uint16_t)(1 + x + (cos(dsec) * rdev));
    ny = (uint16_t)(1 + y + (sin(dsec) * rdev));
    w =  (uint16_t)(1 + x + (cos(dsec) * r));
    h =  (uint16_t)(1 + y + (sin(dsec) * r));
    vga4bit.drawLine(nx, ny, w, h, myColors[7],false);
  }
}

void drawNeedle(uint8_t index, uint16_t bcolor) {
  int16_t i;
  if (curVal[index] == 0) Serial.println("drawing a zero");
  if (oldVal[index] != curVal[index]) {
    if (curVal[index] > oldVal[index]) {
      for (i = oldVal[index]; i <= curVal[index]; i++) {
        if (i > 0) drawPointerHelper(index, i - 1, posx[index], posy[index], radius[index], bcolor);
        drawPointerHelper(index, i, posx[index], posy[index], radius[index], needleColors[index]);
        if ((curVal[index] - oldVal[index]) < (128)) delay(1);//ballistic
      }
    }
    else {
      for (i = oldVal[index]; i >= curVal[index]; i--) {
        drawPointerHelper(index, i + 1, posx[index], posy[index], radius[index], bcolor);
        drawPointerHelper(index, i, posx[index], posy[index], radius[index], needleColors[index]);
        //ballistic
        if ((oldVal[index] - curVal[index]) >= 128) {
          delay(1);
        } else {
          delay(3);
        }
      }
    }
    oldVal[index] = curVal[index];
  if (curVal[index] == 0) Serial.println("FINISHED drawing a zero");
  }
}

void drawPointerHelper(uint8_t index, int16_t val, uint16_t x, uint16_t y, uint16_t r, uint16_t color) {
  float dsec;
  const int16_t minValue = 0;
  const int16_t maxValue = 255;
  if (val > maxValue) val = maxValue;
  if (val < minValue) val = minValue;
  dsec = (((float)(uint16_t)(val - minValue) / (float)(uint16_t)(maxValue - minValue) * degreesVal[index][1]) + degreesVal[index][0]) * (PI / 180);
  uint16_t w = (uint16_t)(1 + x + (cos(dsec) * (r / 1.35)));
  uint16_t h = (uint16_t)(1 + y + (sin(dsec) * (r / 1.35)));
  /*
    min: x:713 | y:63 | w:673 | h:87
    min: x:713 | y:63 | w:754 | h:87
  */
  /*
    if (index == 5){
    Serial.print("x:");
    Serial.print(x);
    Serial.print(" | y:");
    Serial.print(y);
    Serial.print(" | w:");
    Serial.print(w);
    Serial.print(" | h:");
    Serial.print(h);
    Serial.print("\n");
    }
  */
  vga4bit.drawLine(x, y, w, h, color, false);
  vga4bit.fillCircle(x, y, 2, color);
}
