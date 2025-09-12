// 1024x768_Screen_8x8_font_4_windows.ino

//**********************************************************************
// NOTE: This sketch must have dimensions 1024x768 sett in 'VGA_T4_Config.h'.
//       Uncomment MAX_WIDTH and MAX_HEIGHT like this:
//
//         #define MAX_WIDTH (1024/2)
//         #define MAX_HEIGHT 768
//
//         //#define MAX_WIDTH (800/2)
//         //#define MAX_HEIGHT 600
//
//         //#define MAX_WIDTH (640/2)
//         //#define MAX_HEIGHT 480
//
//         //#define MAX_WIDTH (640/2)
//         //#define MAX_HEIGHT 400
//**********************************************************************
//       Leave the rest commented out!!
//**********************************************************************

#include "VGA_4bit_T4.h"
#include "VGA_T4_Config.h"
#include "window.h"

FlexIO2VGA vga4bit;

// Uncomment one of the following screen resolutions. Try them all:)
const vga_timing *timing = &t1024x768x60; // Not enough memory avilable for this screen size in this sketch.
//const vga_timing *timing = &t800x600x60;
//const vga_timing *timing = &t640x480x60;
//const vga_timing *timing = &t640x400x70;

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

#define FONTSIZE 8

vgawin_t *win = NULL;  // Get a pointer to array of window structs.
int err = 0, i = 0;

void setup() {
  // Wait for USB Serial
  while (!Serial && (millis() < 5000)) {
    yield();
  }	
if(CrashReport) Serial.println(CrashReport);
  vga4bit.stop();
  // Setup VGA display: 800x600x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits, Mono not supported yet.
  vga4bit.begin(*timing, false, false, 4);
  // Set fontsize 8x8 or (8x16 available)
  vga4bit.setFontSize(FONTSIZE, false);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLUE); 
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  // Clear screen to background color
  vga4bit.clear(VGA_BLUE);
  // Initialize text cursor:
  //  - block cursor 8 pixels wide and 8 pixels high
  //  - blink enabled
  //  - blink rate of 30
  vga4bit.initCursor(0,0,8,8,false,30);
  // Set white text cursor
  vga4bit.setTcursorColor(VGA_BRIGHT_WHITE);
  // Turn cursor on if active
  vga4bit.tCursorOn();
  // Move cursor to home position
  vga4bit.textxy(0,0);

  win = getWinPointer(); // The window struct array pointer.

  win[0].active = false; // True if window is in focus.
  // Next four are the size of the window (excluding frame and title bar);
  win[0].x1 = 0;//0;         
  win[0].y1 = 0;//0
  win[0].x2 = 62;//62;
  win[0].y2 = 45;//45;
  win[0].save_x = 0; // - (x1+left border (1 * character width))
  win[0].save_y = 0; // - (y1+top border+window title+1 more border (3 * character height))
  win[0].fgc = VGA_BRIGHT_YELLOW; // Text initial foreground color.
  win[0].bgc = VGA_BLUE;        // Text initial background color.
  win[0].fgcSave = VGA_BRIGHT_WHITE; // Temp storage for foreground color.
  win[0].bgcSave = VGA_BLUE;         // Temp storage for background color.
  win[0].frameColor = VGA_BRIGHT_GREEN; // Frame color.
  win[0].frameType = DOUBLE_LINE;    // Frame type single or double lines.
  win[0].shadowEnable = false;        // Window shadow enable.
  win[0].shadowPattern = 176;        // shadow pattern.
  win[0].shadowColor = VGA_WHITE;    // Shadow foreground color.
  win[0].shadowBGColor = VGA_BLACK;  // Shadow backgraound color.
  win[0].titleColor = VGA_BRIGHT_RED; // Widow title color
  win[0].winTitle = (const char *)"TEENSY WINDOW 1 (61x43)"; // Window title (centered).

  win[1].active = false;
  win[1].x1 = 64;//64;
  win[1].y1 = 0;//0;
  win[1].x2 = 126;//126
  win[1].y2 = 45;//45
  win[1].save_x = 0;
  win[1].save_y = 0;
  win[1].fgc = VGA_BRIGHT_GREEN;
  win[1].bgc = VGA_BLUE;
  win[1].fgcSave = VGA_BRIGHT_WHITE;
  win[1].bgcSave = VGA_BLUE;
  win[1].frameColor = VGA_BRIGHT_YELLOW;
  win[1].frameType = DOUBLE_LINE;
  win[1].shadowEnable = false;
  win[1].shadowPattern = 219;
  win[1].shadowColor = VGA_BLACK;
  win[1].shadowBGColor = VGA_BLACK;
  win[1].titleColor = VGA_BRIGHT_RED;
  win[1].winTitle = (const char *)"TEENSY WINDOW 2 (61x43)";

  win[2].active = false; // True if window is in focus.
  // Next four are the size of the window (excluding frame and title bar);
  win[2].x1 = 0;//0;         
  win[2].y1 = 49;//49;
  win[2].x2 = 62;//62;
  win[2].y2 = 92;//92;
  win[2].save_x = 0;
  win[2].save_y = 0;
  win[2].fgc = VGA_BRIGHT_WHITE; // Text initial foreground color.
  win[2].bgc = VGA_BLUE;        // Text initial background color.
  win[2].fgcSave = VGA_BRIGHT_WHITE; // Temp storage for foreground color.
  win[2].bgcSave = VGA_BLUE;         // Temp storage for background color.
  win[2].frameColor = VGA_BRIGHT_CYAN; // Frame color.
  win[2].frameType = DOUBLE_LINE;    // Frame type single or double lines.
  win[2].shadowEnable = false;        // Window shadow enable.
  win[2].shadowPattern = 176;        // shadow pattern.
  win[2].shadowColor = VGA_WHITE;    // Shadow foreground color.
  win[2].shadowBGColor = VGA_BLACK;  // Shadow backgraound color.
  win[2].titleColor = VGA_BRIGHT_RED; // Widow title color
  win[2].winTitle = (const char *)"TEENSY WINDOW 3 (61x41)"; // Window title (centered).

  win[3].active = false; // True if window is in focus.
  // Next four are the size of the window (excluding frame and title bar);
  win[3].x1 = 64;//64;         
  win[3].y1 = 49;//49;
  win[3].x2 = 126;//126;
  win[3].y2 = 92;//92;
  win[3].save_x = 0;
  win[3].save_y = 0;
  win[3].fgc = VGA_BRIGHT_CYAN; // Text initial foreground color.
  win[3].bgc = VGA_BLUE;        // Text initial background color.
  win[3].fgcSave = VGA_BRIGHT_WHITE; // Temp storage for foreground color.
  win[3].bgcSave = VGA_BLUE;         // Temp storage for background color.
  win[3].frameColor = VGA_BRIGHT_MAGENTA; // Frame color.
  win[3].frameType = DOUBLE_LINE;    // Frame type single or double lines.
  win[3].shadowEnable = false;        // Window shadow enable.
  win[3].shadowPattern = 176;        // shadow pattern.
  win[3].shadowColor = VGA_WHITE;    // Shadow foreground color.
  win[3].shadowBGColor = VGA_BLACK;  // Shadow backgraound color.
  win[3].titleColor = VGA_BRIGHT_RED; // Widow title color
  win[3].winTitle = (const char *)"TEENSY WINDOW 4 (61x41)"; // Window title (centered).

  if((err = openWindow(0)) < 0)
    Serial.printf("Window %d is already open or window number is greater then 10\n", 0);

  if((err = openWindow(1)) < 0)
    Serial.printf("Window %d is already open or window number is greater then 10\n", 1);

  if((err = openWindow(2)) < 0)
    Serial.printf("Window %d is already open or window number is greater then 10\n", 2);

  if((err = openWindow(3)) < 0)
    Serial.printf("Window %d is already open or window number is greater then 10\n", 3);
}

int w0 = 0;
int w1 = 0;
int w2 = 0;
int w3 = 0;
uint32_t dly = 200; // delay between window selecting.

void loop() {
  if((err = selectWindow(0)) < 0)
    Serial.printf("Window %d is not open\n", 0);
//waitforInput();
  vga4bit.printf("This window is active... %d\n", w0++); 
  vga4bit.reverseVid(true);
  vga4bit.printf("This window is in-active..."); 
  vga4bit.reverseVid(false);
  vga4bit.printf("\n");
  delay(dly);
  if((err = selectWindow(1)) < 0)
    Serial.printf("Window %d is not open\n", 1);
  vga4bit.printf("This window is active... %d\n", w1++); 
  vga4bit.reverseVid(true);
  vga4bit.printf("This window is in-active..."); 
  vga4bit.reverseVid(false);
  vga4bit.printf("\n");
  delay(dly);

  if((err = selectWindow(2)) < 0)
    Serial.printf("Window %d is not open\n", 2);
  vga4bit.printf("This window is active... %d\n", w2++); 
  vga4bit.reverseVid(true);
  vga4bit.printf("This window is in-active..."); 
  vga4bit.reverseVid(false);
  vga4bit.printf("\n");
  delay(dly);

  if((err = selectWindow(3)) < 0)
    Serial.printf("Window %d is not open\n", 3);
  vga4bit.printf("This window is active... %d\n", w3++); 
  vga4bit.reverseVid(true);
  vga4bit.printf("This window is in-active..."); 
  vga4bit.reverseVid(false);
  vga4bit.printf("\n");
  delay(dly);

}

void waitforInput()
{
  Serial.println("Press anykey to continue");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}

