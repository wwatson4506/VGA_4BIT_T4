// VGA_T4_Scroll_Test.ino
// A simple example of scrolling a print window in four different directions.
// The bigger the print window the slower the scroll:(
// This demo has delays setup so text is not to blurry as it scrolls:)


#include "VGA_4bit_T4.h"

#define FONTSIZE 16

//const vga_timing *timing = &t1024x768x60;
//const vga_timing *timing = &t800x600x60;
const vga_timing *timing = &t640x480x60;
//const vga_timing *timing = &t640x400x70;

// Must use this instance name (vga4bit). It is used in the driver.
FlexIO2VGA vga4bit;

int i=0;

void setup() {
  while(!Serial);
  Serial.printf("%c",12); // Clears terminal screen on VT100 style terminal.

  vga4bit.stop();
  // Setup VGA display: 1024x768x60
  //                    double Width  = false
  //                    double Height = false
  //                    Color Depth   = 4 bits
  vga4bit.begin(*timing, false, false, 4);
  // Set fontsize 8x16 or (8x8 available)
  vga4bit.setFontSize(FONTSIZE, false);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLUE); 
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  // Clear screen to background color
  vga4bit.clear(VGA_BLUE);
  vga4bit.printf("FOUR WAY SCROLLING DEMO\n\n");
  vga4bit.printf("Press any key to continue\n");
  waitforInput();
  vga4bit.setPrintCWindow(1,5,80,25);
  vga4bit.textxy(0,0);
  vga4bit.printf("SLOW SCROLL!!\n"); 
}

int dly = 400; // delay after direction change.
int scldly = 100; // delay between scrolls.

void loop() {
  for(i = 0; i < 65; i++) {
    vga4bit.scrollRightPrintWindow();
    delay(scldly);
  }
  delay(dly);

  for(i = 0; i < 24; i++) {
    vga4bit.scrollDownPrintWindow();
    delay(scldly);
  }
  delay(dly);

  for(i = 0; i < 65; i++) {
    vga4bit.scrollLeftPrintWindow();
    delay(scldly);
  }
  delay(dly);

  for(i = 0; i < 24; i++) {
    vga4bit.scrollUpPrintWindow();
    delay(scldly);
  }
  delay(dly);
}

void waitforInput()
{
  Serial.printf("Press any key to continue\n");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}
