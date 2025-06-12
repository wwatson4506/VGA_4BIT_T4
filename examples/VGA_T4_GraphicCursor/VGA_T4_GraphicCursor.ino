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

#define FONTSIZE 8 // 8 = 8x8 font or 16 = 8x16 font.

// Must use this instance name (vga4bit). It is used in the driver.
FlexIO2VGA vga4bit;
static int fb_width, fb_height; // Frame Buffer size vars.

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
//USBHub hub3(myusb);
//USBHub hub4(myusb);

MouseController mouse1(myusb);
KeyboardController keyboard1(myusb); // Not used.
USBHIDParser hid1(myusb); // Needed for USB mouse.
USBHIDParser hid2(myusb); // Needed for USB wireless keyboard/mouse combo.

// A structure to hold results of mouse operations.
// Some are not used in this sketch.
struct usbMouseMsg_struct {
	uint8_t buttons;
	uint8_t snglClickCnt;
	uint8_t dblClickCnt;
	uint8_t clickCount;
	int8_t mousex;
	int8_t mousey;
	int16_t scaledX;
	int16_t scaledY;
	int8_t wheel;
	int8_t wheelH;
	int16_t mouseXpos;
	int16_t mouseYpos;
	boolean mouseEvent;
};

// An working mouse message object.
usbMouseMsg_struct mouse_msg;

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

// global Variables for scaleMouseXY.
int16_t fine_dx = 0;
int16_t fine_dy = 0;
int16_t event_dx = 0;
int16_t event_dy = 0;
int16_t nevent_dx = 0;
int16_t nevent_dy = 0;

// Adjustable parameters for mouse cursor movement.
int16_t delta  = 127;
int16_t accel  = 5;
int16_t scaleX = 2;
int16_t scaleY = 2;

// Counters for single and doubble clicks.
uint8_t scCount = 0;
uint8_t dcCount = 0;

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
  vga4bit.begin(*timing, false, false, 4);
  // Get display dimensions
  vga4bit.getFbSize(&fb_width, &fb_height);
  // Set fontsize 8x16 or (8x8 available)
  vga4bit.setFontSize(FONTSIZE, false);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLACK); 
  vga4bit.setForegroundColor(VGA_BRIGHT_GREEN);
  // Clear screen to background color
  vga4bit.clear(VGA_BLACK);
  // Startup USBHost
  myusb.begin();
  // Set mouse cursor position to center of screen.
  mouse_msg.scaledX = fb_width/2;
  mouse_msg.scaledY = fb_height/2;
  // Move text cursor to home position
  vga4bit.textxy(0,0);
  // Print a title.
  vga4bit.println("********** USB Mouse and Graphic Cursor Example **********");
  // Draw a centered white filled rectangle.
  vga4bit.fillRect(fb_width/4,fb_height/4,(fb_width/2)+(fb_width/4),
                  (fb_height/2)+(fb_height/4),VGA_BLUE);
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
    vga4bit.moveGcursor(mouse_msg.scaledX, mouse_msg.scaledY); 
    vga4bit.printf("Mouse X: %4.4d\n", mouse_msg.scaledX);
    vga4bit.printf("Mouse Y: %4.4d\n", mouse_msg.scaledY);
    vga4bit.printf("Buttons: %d\n", getMouseButtons());
    vga4bit.printf("Wheel: %2d\n", mouse_msg.wheel);
    vga4bit.printf("WheelH: %2d\n", mouse_msg.wheelH);
/*
    checkMouseClicks(); // Check for mouse button use.  // ***** This method is blocking, DISABLED for now!! *****
	scCount += getSnglClick(); // Add to Single Click Count.
	dcCount += getDblClick(); // Add to Double Click Count.
    vga4bit.printf("Single Clicks: %d\n", scCount);
    vga4bit.printf("Double Clicks: %d\n", dcCount);
*/
	vga4bit.textxy(0,2); // Set print position.
    mouse1.mouseDataClear();
  }
}

// Scale mouse XY to fit our screen.
void scaleMouseXY(void) {
  nevent_dx = (int16_t)mouse1.getMouseX();
  nevent_dy = (int16_t)mouse1.getMouseY();
  if(abs(nevent_dx) + abs(nevent_dy) > delta) {
    nevent_dx *= accel;
    nevent_dy *= accel;
  }
  event_dx += nevent_dx;
  event_dy += nevent_dy;
  fine_dx += event_dx; 
  fine_dy += event_dy; 
  event_dx = fine_dx / scaleX;
  event_dy = fine_dy / scaleY;
  fine_dx %= scaleX;
  fine_dy %= scaleY;
  mouse_msg.scaledX += event_dx;
  mouse_msg.scaledY += event_dy;
  // Clip to display dimensions.
  if(mouse_msg.scaledX < 0) mouse_msg.scaledX = 0;
  if(mouse_msg.scaledX > (uint16_t)fb_width-1) mouse_msg.scaledX = (uint16_t)fb_width-1;
  if(mouse_msg.scaledY < 0)	mouse_msg.scaledY = 0;
  if(mouse_msg.scaledY > (uint16_t)fb_height-1)	mouse_msg.scaledY = (uint16_t)fb_height-1;
}

// Check for a mouse Event
bool mouseEvent(void) {
	if(!mouse1.available())	return false;
	mouse_msg.wheel = (int8_t)mouse1.getWheel(); // Check for wheel movement
	mouse_msg.wheelH = (int8_t)mouse1.getWheelH();
	scaleMouseXY();
	return true;
}

// Check for mouse button presses
uint8_t getMouseButtons(void) {
	mouse_msg.buttons = (uint8_t)mouse1.getButtons();
	return mouse_msg.buttons;
}

// ***************************************************
// * This function is not right. Disabled for now!!! *
// ***************************************************
// A simple routine to detect for mouse button double clicks.
// Single clicks will also be recorded even if a double click is
// detected but can be ignored if needed. 
void checkMouseClicks(void) {
  uint8_t clickCount = 0;
  uint32_t timerStart = 0;
  uint32_t timeOut = 375; // Set this for double click timeout
  bool getOut = false;

  while(!getOut) {
    // Check for Left mouse button double click.
    if((getMouseButtons() & 1) == 1) {
      if(clickCount == 0) {
        mouse_msg.snglClickCnt++; // Single click detected
        timerStart = millis();
      }
      clickCount++;
      if(clickCount >= 2) { // No more than two clicks counts				
        clickCount = 0; 
        mouse_msg.dblClickCnt++; // Double click detected
        getOut = true; // All done. Exit
      }
      while(((getMouseButtons() & 1) != 0)) {  // Wait for button release. This is blocking!!!
        delay(0); // ?
      }
    }
    delay(0); // ?
    if(((millis()-timerStart) > timeOut) || (getOut == true))
      break; // Nothing happened getOut
  }
}

// Check for a double left mouse button click
uint8_t getDblClick(void) {
	uint8_t dClick = mouse_msg.dblClickCnt;
	if(dClick > 0)
		mouse_msg.dblClickCnt = 0;
	return dClick;
}

// Check for a single left mouse button click
uint8_t getSnglClick(void) {
	uint8_t sClick = mouse_msg.snglClickCnt;
	if(sClick > 0)
		mouse_msg.snglClickCnt = 0;
	return sClick;
}
