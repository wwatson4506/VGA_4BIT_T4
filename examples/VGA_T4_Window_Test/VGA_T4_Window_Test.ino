/*
 VGA_T4_Window_Test.ino
 Window testing for 4 bit VGA driver on Teensy 4.1.
*/

#include "VGA_4bit_T4.h"
#include "window.h"

#define FONTSIZE 8

//const vga_timing *timing = &t1024x768x60;
//const vga_timing *timing = &t800x600x60;
const vga_timing *timing = &t640x480x60;
//const vga_timing *timing = &t640x400x70;

// Must use this instance name (vga4bit). It is used in the driver.
FlexIO2VGA vga4bit;
//static int fb_width, fb_height;

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

vgawin_t *win = NULL;  // Get a pointer to array of window structs.
int err = 0, i = 0;
 
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
  // Get display dimensions
//  vga4bit.getFbSize(&fb_width, &fb_height);
  // Set fontsize 8x16 or (8x8 available)
  vga4bit.setFontSize(FONTSIZE, false);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLUE); 
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  // Clear screen to background color
  vga4bit.clear(VGA_BLUE);

  win = getWinPointer(); // The window struct array pointer.

  win[0].active = false; // True if window is in focus.
  // Next four are the size of the window (excluding frame and title bar);
  win[0].x1 = 5;         
  win[0].y1 = 5;
  win[0].x2 = 40;
  win[0].y2 = 40;
  win[0].fgc = VGA_BRIGHT_YELLOW; // Text initial foreground color.
  win[0].bgc = VGA_BLUE;        // Text initial background color.
  win[0].fgcSave = VGA_BRIGHT_WHITE; // Temp storage for foreground color.
  win[0].bgcSave = VGA_BLUE;         // Temp storage for background color.
  win[0].frameColor = VGA_BRIGHT_GREEN; // Frame color.
  win[0].frameType = DOUBLE_LINE;    // Frame type single or double lines.
  win[0].shadowEnable = true;        // Window shadow enable.
  win[0].shadowColor = VGA_WHITE;    // Shadow color.
  win[0].titleColor = VGA_BRIGHT_RED; // Widow title color
  win[0].winTitle = (const char *)"TEENSY WINDOW 1"; // Window title (centered).

  win[1].active = false;
  win[1].x1 = 45;
  win[1].y1 = 15;
  win[1].x2 = 75;
  win[1].y2 = 40;
  win[1].fgc = VGA_BLACK;
  win[1].bgc = VGA_BRIGHT_CYAN;
  win[1].fgcSave = VGA_BRIGHT_WHITE;
  win[1].bgcSave = VGA_BLUE;
  win[1].frameColor = VGA_BLACK;
  win[1].frameType = DOUBLE_LINE;
  win[1].shadowEnable = true;
  win[1].shadowColor = VGA_WHITE;
  win[1].titleColor = VGA_BLUE;
  win[1].winTitle = (const char *)"TEENSY WINDOW 2 ";


  // Create a desktop
  desktop(VGA_CYAN, SINGLE_LINE, VGA_BRIGHT_WHITE, VGA_BRIGHT_YELLOW, (const char *)"T E E N S Y  4.1  D E S K T O P");

  if((err = openWindow(0)) < 0)
    Serial.printf("Window %d is already open or window number is greater then 10\n", 0) ;
  if((err = openWindow(1)) < 0)
    Serial.printf("Window %d is already open or window number is greater then 10\n", 1) ;

  // Initialize text cursor:
  //  - block cursor 8 pixels wide and 8 pixels high
  //  - blink enabled
  //  - blink rate of 30
  vga4bit.initCursor(0,0,8,8,true,30);
  // Set white text cursor
  vga4bit.setTcursorColor(VGA_BRIGHT_WHITE);
  // Turn cursor on if active
  vga4bit.tCursorOn();
  // Move cursor to home position
  vga4bit.textxy(0,0);

  if((err = selectWindow(0)) < 0)
    Serial.printf("Window %d is not open\n", 0);
  clearWindow(0);
  vga4bit.textxy(0,0);
  vga4bit.printf("This window is active.\n"); 

  vga4bit.printf("\nPress any key to continue...\n"); 
  waitforInput();
  for(i = 32; i <= 255; i++)
	vga4bit.printf("%c",i); 
  vga4bit.printf("\n\nPress any key to continue...\n"); 
  waitforInput();

  if((err = selectWindow(1)) < 0)
    Serial.printf("Window %d is not open\n", 1);
  clearWindow(1);
  vga4bit.textxy(0,0);
  vga4bit.printf("This window is active...\n"); 
  vga4bit.printf("Press any key to continue...\n"); 
  waitforInput();
  for(i = 0; i < 1000; i++)
     vga4bit.printf("i = %d,  0x%4.4x\n",i,i);
  vga4bit.printf("\nPress any key to continue...\n"); 
  waitforInput();
  clearWindow(1);

  if((err = selectWindow(0)) < 0)
    Serial.printf("Window %d is not open\n", 0);
  vga4bit.textxy(0,0);
  vga4bit.printf("\nPress any key to continue...\n"); 
  for(int j = 0; j <= 2000; j++) {
    vga4bit.setForegroundColor(random(16));
    vga4bit.setBackgroundColor(random(16));
    i = random(255);
    if(i >= 32) vga4bit.printf("%c",i);
    vga4bit.setForegroundColor(VGA_BRIGHT_GREEN);
    vga4bit.setBackgroundColor(VGA_BLACK);
  }
  vga4bit.printf("\n\nPress any key to continue...\n"); 
  waitforInput();
  clearWindow(0);

  vga4bit.printf("Any key to close both windows...\n"); 
  // Turn cursor off
  vga4bit.tCursorOff();
  waitforInput();
  // Turn cursor off
  vga4bit.tCursorOff();
  windowClose(0);
  windowClose(1);

  vga4bit.setForegroundColor(VGA_BLACK);
  vga4bit.setBackgroundColor(VGA_BRIGHT_WHITE);
  uint8_t x = getCenteredTextX((vga4bit.getTwidth()-2), (const char *)"F I N I S H E D !!");
  vga4bit.textxy(x,vga4bit.getTheight()/2);
  vga4bit.printf("%s",(const char *)"F I N I S H E D !!");
}

void loop() {

}

void waitforInput()
{
  Serial.printf("Press anykey to continue\n");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}
