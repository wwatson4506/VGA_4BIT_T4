#include "VGA_4bit_T4.h"

const vga_timing *timing = &t640x400x70;

// Must use this instance name. It's used in the driver.
FlexIO2VGA vga4bit;
static int fb_width, fb_height;

#define FONTSIZE 8

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
  // Set fontsize 8x8 or (8x16 available)
  vga4bit.setFontSize(FONTSIZE);
  // Setup 320x200 text window.
  vga4bit.setPrintWindow((fb_width-320)/2,(fb_height-200)/2, 320,200);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLUE); 
  vga4bit.setForegroundColor(VGA_BRIGHT_BLUE);
  // Clear graphic screen
  vga4bit.clear(VGA_BRIGHT_BLUE);
  // Clear Print Window
  vga4bit.clearPrintWindow();
  // Initialize text cursor:
  //  - block cursor 7 pixels wide and 8 pixels high
  //  - blink enabled
  //  - blink rate of 30
  vga4bit.initCursor(0,0,7,11,true,30);
  // Turn cursor on
  vga4bit.cursorOn();
  // Move cursor to home position
  vga4bit.textat(0,0);
  // Display a C64 like screen
  vga4bit.printf("\n    **** COMMODORE 64 BASIC V2 ****     \n");
  vga4bit.printf("\n 64K RAM SYSTEM  38911 BASIC BYTES FREE \n");
  vga4bit.printf("\nREADY.\n");
  vga4bit.printf("10 for i = 0 to 255\n");
  vga4bit.printf("20   print i\n");
  vga4bit.printf("30 next i\n");
  vga4bit.printf("40 end\n");
}


void loop() {
}
