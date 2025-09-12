/*
 VGA_T4_Print_Window_demo.ino
 Demonstrates print window setup and usage.
 Available print window commands:
   setPrintWindow(int x, int y, int width, int height)
   clearPrintWindow()
   scrollUpPrintWindow()
   scrollDownPrintWindow()
   scrollRightPrintWindow()
   scrollLeftPrintWindow()
   unsetPrintWindow()
 4 bit VGA driver on Teensy 4.1.
*/

#include "VGA_4bit_T4.h"
#include "box.h"

#define FONTSIZE 8

//const vga_timing *timing = &t1024x768x60;
//const vga_timing *timing = &t800x600x60;
//const vga_timing *timing = &t640x480x60;
const vga_timing *timing = &t640x400x70;

// Must use this instance name (vga4bit). It is used in the driver.
FlexIO2VGA vga4bit;

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

uint8_t *buf1, *buf2;

void setup() {
  Serial.begin(9600);
//  Serial.print(CrashReport);
  Serial.printf("%c",12);
  vga4bit.stop();
  // Setup VGA display: 1024x768x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits
  vga4bit.begin(*timing, false, false, 4);
  // Set fontsize 8x16 or (8x8 available)
  vga4bit.setFontSize(16, false); // If true, clear screen. If false don't.
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLUE); 
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  // Clear screen to background color
  vga4bit.clear(VGA_BLUE);
  vga4bit.textxy(20,0); // Set text position.
  vga4bit.setForegroundColor(VGA_BRIGHT_GREEN);
  vga4bit.printf("PRINT WINDOW DEMO\n");
  // Set fontsize 8x16 or (8x8 available)
  vga4bit.setFontSize(FONTSIZE, false);
  vga4bit.setForegroundColor(VGA_BRIGHT_YELLOW);
  vga4bit.textxy(7,3);
  vga4bit.printf("Demonstrates setting up a print window framed\n");
  vga4bit.textxy(7,4);
  vga4bit.printf("by a graphic double line box and scrolling text\n");
  vga4bit.textxy(7,5);
  vga4bit.printf("within the boundries of the print window.\n");
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  vga4bit.setBackgroundColor(VGA_CYAN); 
  box_draw(9, 9, 26+5, 50, 2); // last parameter: 1 = single line frame
                               //                 2 = double line frame
  vga4bit.setPrintCWindow(10,10,40,21);
  vga4bit.setBackgroundColor(VGA_BLACK);
  vga4bit.clearPrintWindow();
}

void loop() {
  vga4bit.setForegroundColor(random(16)); // Set random foreground colors.
  vga4bit.printf("%c",random(33)+32); // Print random characters.
  delay(4); // Set to zero to see fastest scroll speed.
}

void waitforInput() {
Serial.printf("Press anykey to continue\n");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}
