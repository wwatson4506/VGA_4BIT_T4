/*
 VGA_T4_treedee.ino
 Taken from Sumotoy's RA8875 library and adapted for
 4 bit VGA driver on Teensy 4.1.
*/

#include "VGA_4bit_T4.h"
#include <math.h>

#define FONTSIZE 16

// Uncomment one of the following screen resolutions. Try them all:)
//const vga_timing *timing = &t1024x768x60;
const vga_timing *timing = &t800x600x60;
//const vga_timing *timing = &t640x480x60;
//const vga_timing *timing = &t640x400x70;

// Must use this instance name (vga4bit). It is used in the driver.
FlexIO2VGA vga4bit;

uint16_t fbWidth, fbHeight;

// Array of vga4bit Basic Colors
const uint8_t myColors[] = {
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

float sin_d[] = { 
  0,0.17,0.34,0.5,0.64,0.77,0.87,0.94,0.98,1,0.98,0.94,
  0.87,0.77,0.64,0.5,0.34,0.17,0,-0.17,-0.34,-0.5,-0.64,
  -0.77,-0.87,-0.94,-0.98,-1,-0.98,-0.94,-0.87,-0.77,
  -0.64,-0.5,-0.34,-0.17 };
float cos_d[] = { 
  1,0.98,0.94,0.87,0.77,0.64,0.5,0.34,0.17,0,-0.17,-0.34,
  -0.5,-0.64,-0.77,-0.87,-0.94,-0.98,-1,-0.98,-0.94,-0.87,
  -0.77,-0.64,-0.5,-0.34,-0.17,0,0.17,0.34,0.5,0.64,0.77,
  0.87,0.94,0.98};
float d = 30;
float px[] = { 
  -d,  d,  d, -d, -d,  d,  d, -d };
float py[] = { 
  -d, -d,  d,  d, -d, -d,  d,  d };
float pz[] = { 
  -d, -d, -d, -d,  d,  d,  d,  d };

float p2x[] = {
  0,0,0,0,0,0,0,0};
float p2y[] = {
  0,0,0,0,0,0,0,0};

int r[] = {
  0,0,0};

uint8_t ch = 15; // Default line color
uint8_t ccolor = myColors[ch];
//delay between interations 
uint8_t speed = 30;

void setup() {
  Serial.begin(9600);
  vga4bit.stop();
  // Setup VGA display: 1024x768x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits
  vga4bit.begin(*timing, false, false, 4);
  // Get display dimensions
  vga4bit.getFbSize(&fbWidth, &fbHeight);
  // Set fontsize 8x16 or (8x8 available)
  vga4bit.setFontSize(FONTSIZE, false);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLACK); 
  vga4bit.setForegroundColor(VGA_BRIGHT_GREEN);
  // Clear screen to background color
  vga4bit.clear(VGA_BLACK);
  vga4bit.textxy(0,fbHeight-FONTSIZE);
  vga4bit.printf("4 bit VGA version of Sumotoy's treedee example at 800x600 with 16 colors...");
}

void loop() {
  r[0] = r[0] + 1;
  r[1] = r[1] + 1;
  if (r[0] == 36) r[0] = 0;
  if (r[1] == 36) r[1] = 0;
  if (r[2] == 36) r[2] = 0;
  for (int i = 0; i < 8; i++)
  {
    float px2 = px[i];
    float py2 = cos_d[r[0]] * py[i] - sin_d[r[0]] * pz[i];
    float pz2 = sin_d[r[0]] * py[i] + cos_d[r[0]] * pz[i];

    float px3 = cos_d[r[1]] * px2 + sin_d[r[1]] * pz2;
    float py3 = py2;
    float pz3 = -sin_d[r[1]] * px2 + cos_d[r[1]] * pz2;

    float ax = cos_d[r[2]] * px3 - sin_d[r[2]] * py3;
    float ay = sin_d[r[2]] * px3 + cos_d[r[2]] * py3;
    float az = pz3 - 190;

    p2x[i] = ((fbWidth) / 2) + ax * 500 / az;
    p2y[i] = ((fbHeight) / 2) + ay * 500 / az;
  }
  for (int i = 0; i < 3; i++) {
    vga4bit.drawLine(p2x[i], p2y[i], p2x[i + 1], p2y[i + 1], ccolor, false);
    vga4bit.drawLine(p2x[i + 4], p2y[i + 4], p2x[i + 5], p2y[i + 5], ccolor, false);
    vga4bit.drawLine(p2x[i], p2y[i], p2x[i + 4], p2y[i + 4], ccolor, false);
  }
  vga4bit.fbUpdate(true); //wait_for_frame();
  vga4bit.drawLine(p2x[3], p2y[3], p2x[0], p2y[0], ccolor, false);
  vga4bit.drawLine(p2x[7], p2y[7], p2x[4], p2y[4], ccolor, false);
  vga4bit.drawLine(p2x[3], p2y[3], p2x[7], p2y[7], ccolor, false);
  vga4bit.fbUpdate(true); //wait_for_frame();
  delay(speed);
  for (int i = 0; i < 3; i++) {
    vga4bit.drawLine(p2x[i], p2y[i], p2x[i + 1], p2y[i + 1], myColors[0], false);
    vga4bit.drawLine(p2x[i + 4], p2y[i + 4], p2x[i + 5], p2y[i + 5], myColors[0], false);
    vga4bit.drawLine(p2x[i], p2y[i], p2x[i + 4], p2y[i + 4], myColors[0], false);
  }
  vga4bit.fbUpdate(false); // Don't wait_for_frame();
  vga4bit.drawLine(p2x[3], p2y[3], p2x[0], p2y[0], myColors[0], false);
  vga4bit.drawLine(p2x[7], p2y[7], p2x[4], p2y[4], myColors[0], false);
  vga4bit.drawLine(p2x[3], p2y[3], p2x[7], p2y[7], myColors[0], false);
  vga4bit.fbUpdate(false); // Don't wait_for_frame();

#if 1 // change to #if 0 for defalt colored frames (VGA_BRIGHT_WHITE)
  if (ch >= 15) {
    ch = 1;
    ccolor = myColors[ch];
    ccolor = myColors[random(1, 15)];
  }
  else {
    ch++;
  }
#endif
}
