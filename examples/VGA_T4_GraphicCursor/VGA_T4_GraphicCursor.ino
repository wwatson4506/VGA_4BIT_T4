/*
 VGA_T4_GraphicCursor.ino 
 Shows usage of a software mouse driven graphic cursor. Also displays
 cursor coordinates, button presses, wheel movement and left mouse
 button single and double click counts. The click counter function is
 blocking so no mouse movement happens with left mouse button held
 down. 
 
 One change has to be made to "mouse.cpp" in the USBHost_t36
 library for mouse single and double click counting to work.
 In the function:
  void MouseController::mouseDataClear() {
	mouseEvent = false;
//	buttons = 0;
	mouseX  = 0;
	mouseY  = 0;
	wheel   = 0;
	wheelH  = 0;
 }
 "buttons = 0;" needs to be commented out otherwise single and double
 clicking will not work. It not necessary to clear buttons as releasing
 the button clears it. Also "wheel" and "wheelH" values are cleared
 automatically when there is any movement of the mouse. 

 The graphic cursor works with all of the 4 supported screen resolutions.
*/

#include "USBHost_t36.h"
#include "VGA_4bit_T4.h"
#include "USBmouseVGA.h"

#define FONTSIZE 16 // 8 = 8x8 font or 16 = 8x16 font.

// Must use this instance name (vga4bit). It is used in the driver.
FlexIO2VGA vga4bit;
uint16_t fbWidth, fbHeight; // Frame Buffer size vars.

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHIDParser hid1(myusb); // Needed for USB mouse.
USBHIDParser hid2(myusb); // Needed for USB wireless keyboard/mouse combo.

// An working mouse message object.
usbMouseMsg *mouse_mesg = getMouseMsgPointer();

// Array of vga4bit basic colors
const uint8_t myColors[] = {
  VGA_BLACK,          // 0
  VGA_BLUE,           // 1
  VGA_GREEN,          // 2
  VGA_CYAN,           // 3
  VGA_RED,            // 4
  VGA_MAGENTA,        // 5
  VGA_YELLOW,         // 6
  VGA_WHITE,          // 7
  VGA_GREY,           // 8
  VGA_BRIGHT_BLUE,    // 9
  VGA_BRIGHT_GREEN,   // 10
  VGA_BRIGHT_CYAN,    // 11
  VGA_BRIGHT_RED,     // 12
  VGA_BRIGHT_MAGENTA, // 13
  VGA_BRIGHT_YELLOW,  // 14
  VGA_BRIGHT_WHITE    // 15
};
// Uncomment one of the following screen resolutions. Try them all:)
//const vga_timing *timing = &t1024x768x60;
//const vga_timing *timing = &t800x600x60;
const vga_timing *timing = &t640x480x60;
//const vga_timing *timing = &t640x400x70;

void setup() {
  Serial.begin(9600);
  // Setup VGA display: 
  //   *timing       = one of the above screen modes
  //   double Height = false
  //   double Width  = false
  //   Color Depth   = 4 bits

Serial.printf("%c\n",12);
if(CrashReport) Serial.println(CrashReport);

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

  // Startup USBHost and mouse driver.
  mouse_init();
  // Set mouse cursor position to center of screen.
  mouse_mesg->scaledX = fbWidth/2;
  mouse_mesg->scaledY = fbHeight/2;
  // Move text cursor to home position
  vga4bit.textxy(0,0);
  // Print a title.
  vga4bit.println("********** USB Mouse and Graphic Cursor Example **********");
  // Draw a centered white filled rectangle.
  vga4bit.fillRect(fbWidth/4,fbHeight/4,(fbWidth/2)+(fbWidth/4),
                  (fbHeight/2)+(fbHeight/4),VGA_BLUE);
  // Init graphic cursor:
  // initGcursor(Cursor Type, Cursor x start, Cursor y start, Cursor x end, Cursor y end) 
  // Cursor type:
  //              0 = block type where last four arguments will set the dimensions
  //                  of the the block cursor.   (BLOCK_CURSOR)
  //              1 = filled white arrow cursor  (FILLED_CURSOR)
  //              2 = hollow white arrow cursor. (HOLLOW_CURSOR)
  //              3 = I-Beam cursor.             (I_BEAM)
  //              (cursors 1, 2 and 3 ignore last for arguments).
  // Block cursor dimensions are in pixels. Cannot exceed font dimensions minus 1.
  // Cursor 
  vga4bit.initGcursor(FILLED_ARROW,0,0,vga4bit.getFontWidth()-1,vga4bit.getFontHeight()-1);
  // Turn graphic cursor on.
  vga4bit.gCursorOn();
  vga4bit.textxy(0,2); // Set print position.
}

void loop() {
  myusb.Task();
  if(mouseEvent()) { // Wait for a mouse event.
    // Set mouse cursor position.
    vga4bit.printf("Mouse X: %4.4d\n", mouse_mesg->scaledX);
    vga4bit.printf("Mouse Y: %4.4d\n", mouse_mesg->scaledY);
    vga4bit.printf("Buttons: %d\n", getMouseButtons());
    vga4bit.printf("Wheel: %2d\n", mouse_mesg->wheel);
    vga4bit.printf("WheelH: %2d\n", mouse_mesg->wheelH);
    vga4bit.moveGcursor(mouse_mesg->scaledX, mouse_mesg->scaledY); 
	vga4bit.textxy(0,2); // Set print position.
    mouseDataClear();
  }
}
