// get_key_mouse_testing.ino

#include "USBHost_t36.h"
#include "USBKeyboard.h"
#include "USBmouseVGA.h"
#include "getKeyMouse.h"
#include "VGA_4bit_T4.h"
#include "VGA_T4_Config.h"
//#include "window.h"
//#include "box.h"
//#include "menu.h"

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);

USBHIDParser hid1(myusb); // Needed for USB mouse.
USBHIDParser hid2(myusb); // Needed for USB wireless keyboard/mouse combo.

FlexIO2VGA vga4bit;

// Uncomment one of the following screen resolutions. Try them all:)
//const vga_timing *timing = &t1024x768x60; // Not enough memory avilable for this screen size in this sketch.
//const vga_timing *timing = &t800x600x60;
//const vga_timing *timing = &t640x480x60;
const vga_timing *timing = &t640x400x70;

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

#define FONTSIZE 16

void setup() {
  // Wait for USB Serial
  while (!Serial && (millis() < 5000)) {
    yield();
  }	

  Serial.printf("%c",12);

  if(CrashReport) Serial.println(CrashReport);


  // Startup USBHost
  myusb.begin();
  delay(500);
  myusb.Task();

  USBKeyboardInit();
//  mouse_init();
   
  vga4bit.stop();
  // Setup VGA display: 800x600x60
  //                    double Width  = false
  //                    double Height = false
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
  vga4bit.initCursor(0,0,7,15,true,30);
  // Set white text cursor
  vga4bit.setTcursorColor(VGA_BRIGHT_WHITE);
  // Move cursor to home position
  vga4bit.textxy(0,0);
  // Turn cursor on if active
  vga4bit.tCursorOn();

  vga4bit.printf("%cA Simple Test Of Using Keyboard And Mouse Together\n",12);
  vga4bit.printf("Uses the 'getkey_or_mouse()' function. Mouse mimics arrow\n");
  vga4bit.printf("keys. Right mouse button mimics ESC key and left mouse\n");
  vga4bit.printf("button mimics the enter key.\n");
  vga4bit.printf("The 'getkey_or_mouse()' function is used in the 'VGA_T4MenuTest.ino'\n");
  vga4bit.printf("example sketch in this library as a simple menu system.\n");

}

void loop() {
  uint16_t key;
  int i;

  for (i = 0; i < 2; i++) {
	if(i == 0) {
	  vga4bit.printf("\nTesting 'getKey()'...\n");
	  vga4bit.printf("Press keys to see ascii codes returned...\n");
	  vga4bit.printf("(press ESC to test both keyboard and mouse)...\n\n");
	} else {
	  vga4bit.printf("\nTesting 'getkey_or_mouse()'...\n");
	  vga4bit.printf("Now press keys or use the mouse...\n");
	  vga4bit.printf("(press ESC or right mouse button to quit)...\n\n");
	}
	do {
	  if (i == 0) {
	    key = USBGetkey();
	  } else {
	    key = getkey_or_mouse();
	  }
	    if( key == MY_KEY_F1 ) vga4bit.printf("KEY_F1        ");
	    if( key == MY_KEY_F2 ) vga4bit.printf("KEY_F2        ");
	    if( key == MY_KEY_F3 ) vga4bit.printf("KEY_F3        ");
	    if( key == MY_KEY_F4 ) vga4bit.printf("KEY_F4        ");
	    if( key == MY_KEY_F5 ) vga4bit.printf("KEY_F5        ");
	    if( key == MY_KEY_F6 ) vga4bit.printf("KEY_F6        ");
	    if( key == MY_KEY_F7 ) vga4bit.printf("KEY_F7        ");
	    if( key == MY_KEY_F8 ) vga4bit.printf("KEY_F8        ");
	    if( key == MY_KEY_F9 ) vga4bit.printf("KEY_F9        ");
	    if( key == MY_KEY_F10 ) vga4bit.printf("KEY_F10       ");
	    if( key == MY_KEY_F11 ) vga4bit.printf("KEY_F11       ");
	    if( key == MY_KEY_F12 ) vga4bit.printf("KEY_F12       ");
        if(key < 194 || key > 205) {
		  vga4bit.printf("\t%u\t%c",key,key);
		  if(key == MY_KEY_UP) vga4bit.printf("\tUP ARROW KEY");
		  if(key == MY_KEY_DOWN)  vga4bit.printf("\tDOWN ARROW KEY");
		  if(key == MY_KEY_LEFT)  vga4bit.printf("\tLEFT ARROW KEY");
		  if(key == MY_KEY_RIGHT) vga4bit.printf("\tRIGHT ARROW KEY");
		  vga4bit.printf("\n");
		} else {
		  vga4bit.printf("\t%u\n",key);
		}	
	} while(key != MY_KEY_ESCAPE);
  }
}

void waitforInput()
{
  Serial.printf("press any key to continue...\n");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}

