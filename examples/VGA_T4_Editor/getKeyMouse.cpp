#include "USBHost_t36.h"
#include "USBKeyboard.h"
#include "getKeyMouse.h"
#include "USBmouseVGA.h"
#include "VGA_4bit_T4.h"
#include "VGA_T4_Config.h"

#define HORZ_COUNTS 30
#define VERT_COUNTS 20

// Get a pointer to the mouse message struct.
usbMouseMsg *mouseMsg = getMouseMsgPointer();

static int mouse_flag = 0;

// Wait for a key press.
uint16_t getkey(void)
{
	uint16_t key;
	key = USBGetkey();
	return(key);

}

// Wait for a key press or mouse event.
uint16_t getkey_or_mouse(void) {
	uint16_t key = 0;
	uint8_t presses = 0;
	int tot_horz = 0;
	int tot_vert = 0;

	// Set the mouse motion to 0
	tot_horz = tot_vert = 0;

	// Clear out the mouse button press counts //
    mouseDataClear(); // We are done clear mouse data.

	// loop starts here, watches for keypress or mouse activity
	while(1) {
		switch(mouse_flag) {
			// If this is first interation, check for existance of mouse
			case 0:
				mouse_flag = 1;
				break;
			// If mouse does not exist, ignore monitoring functions
			case -1:
				break;
			// Check for mouse activity
			case 1:
				// accumulate mouse motion counts
				if(mouseEvent() == true) {  // Any kind of mouse data available?
				  tot_horz += mouseMsg->mouseXpos; // Get raw mouse X motion count +/-.
				  tot_vert += mouseMsg->mouseYpos; // Get raw mouse Y motion count +/-.
				  presses = getMouseButtons(); // Grab any button presses.
				  mouseDataClear(); // We are done clear mouse data.
			    }
				// Check for enough horizontal motion
				if(tot_horz < -HORZ_COUNTS)
					return(MY_KEY_LEFT); // Emulate left key press.
				if(tot_horz > HORZ_COUNTS)
					return(MY_KEY_RIGHT); // Emulate right key press.
				// Check for enough vertical motion
				if(tot_vert < -VERT_COUNTS)
					return(MY_KEY_UP); // Emulate up key press.
				if(tot_vert > VERT_COUNTS)
					return(MY_KEY_DOWN); // Emulate down key press.
				// Check for left or right mouse button presses
				  if(presses & 1) return(MY_KEY_ENTER); //left mouse button
				  if(presses & 2) return(MY_KEY_ESCAPE);//Right mouse button
				break;
		}
		// Check for keyboard input
		if(USBKeyboard_available()) {
   		  key = USBGetkey();
   		  return(key);
   		}
	}
}
