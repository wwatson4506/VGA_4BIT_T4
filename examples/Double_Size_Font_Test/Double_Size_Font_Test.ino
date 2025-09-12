// Double_Size_Font_test.ino

//**********************************************************************
// NOTE: This sketch must have dimensions 1024x768 set in 'VGA_T4_Config.h'.
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

const vga_timing *timing4 = &t1024x768x60;
const vga_timing *timing3 = &t800x600x60;
const vga_timing *timing2 = &t640x480x60;
const vga_timing *timing1 = &t640x400x70;

// Must use this instance name. It's used in the driver.
FlexIO2VGA vga4bit;
uint16_t fbWidth, fbHeight;

#define FONTSIZE 16
int i = 0;

void setup() {
  Serial.begin(9600);

  // Stop dislay if running to setup Display.
  vga4bit.stop();

  // Setup VGA display: 640x400x70
  //                    double Width  = false
  //                    double Height = false
  //                    Color Depth   = 4 bits, Only 4 bit mode supported. 1 Bit mode WIP.
  vga4bit.begin(*timing1, false, false, 4); // Default.

  // Get display dimensions
  vga4bit.getFbSize(&fbWidth, &fbHeight);

  // Set fontsize 8x8 or (8x16 available)
  vga4bit.setFontSize(FONTSIZE,false);

  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLUE); 
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);

  // Clear graphic screen
  vga4bit.clear(VGA_BLUE);

  // Move cursor to home position
  vga4bit.textxy(0,0);

 // Initialize text cursor:
  //  - block cursor 7 pixels wide and 8 pixels high
  //  - blink enabled
  //  - blink rate of 30
  vga4bit.initCursor(0,0,7,11,true,30);

  // Turn cursor on
  vga4bit.tCursorOn();

  // Timing mode 1 600x400
  vga4bit.setScreenMode(*timing1, false, false, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 600x400\n");
  vga4bit.printf(" Double Width mode: false\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing1, true, false, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 600x400\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing1, true, true, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 600x400\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: true\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing1, false, false, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 600x400\n");
  vga4bit.printf(" Double Width mode: false\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing1, true, false, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 600x400\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing1, true, true, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 600x400\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: true\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  // Timing mode 2 600x480
  vga4bit.setScreenMode(*timing1, false, false, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 600x480\n");
  vga4bit.printf(" Double Width mode: false\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing2, true, false, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 600x480\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing2, true, true, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 600x480\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: true\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing2, false, false, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 600x480\n");
  vga4bit.printf(" Double Width mode: false\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing2, true, false, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 600x480\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing2, true, true, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 600x480\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: true\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  // Timing mode 3 800x600
  vga4bit.setScreenMode(*timing3, false, false, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 800x600\n");
  vga4bit.printf(" Double Width mode: false\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing3, true, false, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 800x600\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing3, true, true, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 800x600\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: true\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing3, false, false, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 800x600\n");
  vga4bit.printf(" Double Width mode: false\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing3, true, false, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 800x600\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing3, true, true, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 800x600\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: true\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  // Timing mode 4 1024x768
  vga4bit.setScreenMode(*timing4, false, false, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 1024x768\n");
  vga4bit.printf(" Double Width mode: false\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing4, true, false, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 1024x768\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing4, true, true, 4);
  vga4bit.setFontSize(8,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x8\n");
  vga4bit.printf("       Screen mode: 1024x768\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: true\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing4, false, false, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 1024x768\n");
  vga4bit.printf(" Double Width mode: false\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing4, true, false, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 1024x768\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: false\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  waitforInput();

  vga4bit.setScreenMode(*timing4, true, true, 4);
  vga4bit.setFontSize(16,false);
  vga4bit.textxy(0,0);
  vga4bit.printf("         Font size: 8x16\n");
  vga4bit.printf("       Screen mode: 1024x768\n");
  vga4bit.printf(" Double Width mode: true\n");
  vga4bit.printf("Double Height mode: true\n");
  vga4bit.printf("   Color bit depth: always 4\n");
  vga4bit.printf("\nDemo is done...\n");
}

void loop() {
//vga4bit.printf("testing...\n");
delay(500);

// Hang out here...
}

void waitforInput()
{
  Serial.printf("Press any key to continue\n");
  vga4bit.printf("Press a key for next screen type\n");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}
