//=============================
// USBkeyboard.cpp
// By: Warren Watson 2017-2023
//=============================
#include "VGA_4bit_T4.h"
#include "VGA_T4_Config.h"
#include "elapsedMillis.h"
#include "USBHost_t36.h"
#include "USBKeyboard.h"
#include "IntervalTimer.h"

extern USBHost myusb;
extern FlexIO2VGA vga4bit;

KeyboardController keyboard1(myusb);

//**********************************************************************
// The folowing two lines must be included in the main sketch when using
// the USB keyboard or USB mouse even if using just one of the two:
//
//     USBHIDParser hid1(myusb);
//     USBHIDParser hid2(myusb);
//**********************************************************************

IntervalTimer keyrepeat;		//Setup Key Repeat Timer
elapsedMillis keypresselapsed;	//Setup Key Press Repeat Start Time Interval

uint8_t ctrl_c_flag;
uint8_t xon_xoff_flag;

usbKeyMsg_t keyboard_msg;
usbKeyMsg_t *keyboard_msg_p = &keyboard_msg;


#define BUFFER_SIZE 45
static volatile int buffer[BUFFER_SIZE];
static volatile int sbuffer[BUFFER_SIZE];
static volatile uint8_t head, tail;


//=============================
// Get a keypress. Wait for it.
//=============================
uint16_t USBGetkey(void) {
	while(!USBKeyboard_available());
	return USBKeyboard_read();
}

//===============================
// Check key buffer for keypress.
// Return true if available.
//===============================
int USBKeyboard_available(void) {
	if (head != tail && buffer[head] != 0)
	   return true;
	else
		return false;
}

//===============================
// Check scan code buffer for a
// scan code.
//===============================
int USBScancode_available(void) {
	if (head != tail && sbuffer[head] != 0)
		return true;
	else
		return false;
}

//===============================
// Read a keypress from buffer.
//===============================
int USBKeyboard_read(void) {
	uint8_t c, i;

	i = tail;
	if (i == head) return 0;
	i++;
	if (i >= BUFFER_SIZE) i = 0;
	c = buffer[i];
	tail = i;
	return c;
}

//===============================
// Read a scancoded from the scan
// code buffer.
//===============================
uint16_t USBKeyboard_readscancode(void) {
	uint8_t c, i;

	i = tail;
	if (i == head) return 0;
	i++;
	if (i >= BUFFER_SIZE) i = 0;
	c = sbuffer[i];
	tail = i;
	return c;
}

//==================
// Get OEM keypress.
//==================
uint8_t USBGetOemKey(void) {
	return keyboard_msg_p->keyOEM;
}

//==================
// Get modifiers.
//==================
uint8_t USBGetModifiers(void) {
	return keyboard_msg_p->modifiers;
}

//=======+++++++++++++===========
// Check if keypress is held long
// enough to do auto repeat.
//===============================
void checkKeyRepeat(void) {
	if(keyboard_msg_p->keypressed == 1 &&
	   keypresselapsed > keyboard_msg_p->keyRepeatDelay) {
		writeBuff();
	}	
	else
		return;
}

//===============================
// Write key code to buffer and
// scan code to sbuffer.
//===============================
void writeBuff(void) {
	uint8_t i = 0;

	i = head + 1;	//Get current buffer pointer
	if(i >= BUFFER_SIZE) i = 0; // Wrap around if buffer is full
	if (i != tail) {
		buffer[i] = keyboard_msg_p->keyCode;
		sbuffer[i] = keyboard_msg_p->keyOEM;
		head = i;
	}
}

//==============================
// Key press interrupt. Store
// results to struct and then
// write to buffers.
//==============================
void OnPress(int key) {
	keypresselapsed = 0;
	keyboard_msg_p->keypressed = 1;
	keyboard_msg_p->keyCode = keyboard1.getKey();
	keyboard_msg_p->modifiers = keyboard1.getModifiers();
	keyboard_msg_p->keyOEM = keyboard1.getOemKey();
	writeBuff();
}

//===============================
// Key release interrupt. Signal
// key press completion and clear
// keyboard message struct.
//===============================
void OnRelease(int key)
{
	if(keyboard_msg_p->keypressed == 1) {
		// flush keyboard buffer
 		head = tail = 0;
		buffer[head] = 0;
	}
	keyboard_msg_p->keypressed = 0;
	keyboard_msg_p->keyOEM = 0;
	keyboard_msg_p->last_keyCode = key;
	keyboard_msg_p->keyCode = 0;
	keyboard_msg_p->modifiers = 0;
}

//--------------------------------
// Return a scancode or a keypress
//--------------------------------
uint8_t getSCorKP(bool scORkp) {
	uint8_t scode = 0;
	if(scORkp) { 
		if(USBScancode_available()) {
			scode = USBKeyboard_readscancode();
			if(scode < 4 || scode > 39)
				return scode;
			else
				return 0;
		}
	}
	scode = (uint8_t)USBKeyboard_read();
	return scode;	
}

//---------------------------------------------------------------------
// Get a string from keyboard. Process control codes and function keys.
//---------------------------------------------------------------------
char *cgets(char *s)
{
	char *s1 = s;
	int	c;
	for(;;) {
		switch(c = (uint8_t)USBGetkey()) {
		case 194: // F1   Key
		case 195: // F2   Key
		case 196: // F3   Key
		case 197: // F4   Key
		case 198: // F5   Key
		case 199: // F6   Key
		case 200: // F7   Key
		case 201: // F8   Key
		case 202: // F9   Key
		case 203: // F10  Key
		case 204: // F11  Key
		case 205: // F12  Key
			*s = c;
			return s;
		case 127:
			if(s == s1)
				break;
			s1--;
			vga4bit.printf("%c",c);
			break;
		case '\n':
		case '\r':
			vga4bit.printf("%c",c);
			*s1 = 0;
			return s;
		default:
			if(c <= ASCII_7E)
			vga4bit.printf("%c",c);
			*s1++ = c;
			break;
		}
	}
}

//==============================
// Init USB keyboard system.
//==============================
void USBKeyboardInit(void) {
	keyboard1.attachPress(OnPress);
	keyboard1.attachRelease(OnRelease);
	head = 0;
	tail = 0;
	ctrl_c_flag = 0;
	keyboard_msg_p->keyRepeatDelay = 800; // Default repeat delay
	keyboard_msg_p->keyRepeatRate = 500; // Default repeat rate 
	keyrepeat.begin(checkKeyRepeat,(unsigned long)(keyboard_msg_p->keyRepeatRate * 100));
}
