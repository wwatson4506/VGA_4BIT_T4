//mouseFunc.h
#ifndef USBMOUSEV2_H
#define USBMOUSEV2_H

#include "USBHost_t36.h"
#include "VGA_4bit_T4.h"

// A structure to hold results of mouse operations.
// Some are not used in this sketch.
typedef struct usbMouseMsg{
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
} usbMouseMsg;

usbMouseMsg *getMouseMsgPointer(void);
int mouse_init(void);
void scaleMouseXY(void);
bool mouseEvent(void);
uint8_t getMouseButtons(void);
void checkMouseClicks(void);
uint8_t getDblClick(void);
uint8_t getSnglClick(void);
void mouseDataClear(void);
#endif
