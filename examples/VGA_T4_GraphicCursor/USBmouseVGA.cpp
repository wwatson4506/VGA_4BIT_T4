// mouseFunc.cpp

#include "USBmouseVGA.h"

// An working mouse message object.
struct usbMouseMsg mmsg;
usbMouseMsg *mouse_msg = &mmsg;

extern USBHost myusb;
MouseController mouse1(myusb);

// Must use this instance name (vga4bit). It is used in the driver.
extern FlexIO2VGA vga4bit;
uint16_t fb_width, fb_height; // Frame Buffer size vars.

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

usbMouseMsg *getMouseMsgPointer(void) {
	return &mmsg;
}

int mouse_init(void) {
  // Get display dimensions
  vga4bit.getFbSize(&fb_width, &fb_height);
  // Startup USBHost
  myusb.begin();
  myusb.Task();

  // Set mouse cursor position to center of screen.
  mouse_msg->scaledX = fb_width/2;
  mouse_msg->scaledY = fb_height/2;
  return 1;
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
  mouse_msg->scaledX += event_dx;
  mouse_msg->scaledY += event_dy;
  // Clip to display dimensions.
  if(mouse_msg->scaledX < 0) mouse_msg->scaledX = 0;
  if(mouse_msg->scaledX > (uint16_t)fb_width-1) mouse_msg->scaledX = (uint16_t)fb_width-1;
  if(mouse_msg->scaledY < 0)	mouse_msg->scaledY = 0;
  if(mouse_msg->scaledY > (uint16_t)fb_height-1)	mouse_msg->scaledY = (uint16_t)fb_height-1;
}

// Check for a mouse Event
bool mouseEvent(void) {
	if(!mouse1.available())	return false;
    mouse_msg->mouseXpos = (int16_t)mouse1.getMouseX();
    mouse_msg->mouseYpos = (int16_t)mouse1.getMouseY();    
	mouse_msg->wheel = (int8_t)mouse1.getWheel(); // Check for wheel movement
	mouse_msg->wheelH = (int8_t)mouse1.getWheelH();
	scaleMouseXY();
	return true;
}

// Check for mouse button presses
uint8_t getMouseButtons(void) {
	mouse_msg->buttons = (uint8_t)mouse1.getButtons();
	return mouse_msg->buttons;
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
        mouse_msg->snglClickCnt++; // Single click detected
        timerStart = millis();
      }
      clickCount++;
      if(clickCount >= 2) { // No more than two clicks counts				
        clickCount = 0; 
        mouse_msg->dblClickCnt++; // Double click detected
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
	uint8_t dClick = mouse_msg->dblClickCnt;
	if(dClick > 0)
		mouse_msg->dblClickCnt = 0;
	return dClick;
}

// Check for a single left mouse button click
uint8_t getSnglClick(void) {
	uint8_t sClick = mouse_msg->snglClickCnt;
	if(sClick > 0)
		mouse_msg->snglClickCnt = 0;
	return sClick;
}

void mouseDataClear(void) {
    mouse1.mouseDataClear();	
}
