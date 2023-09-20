//=============================
// USBKeyboard.h
// By: Warren Watson 2017-2023
//=============================
#ifndef USBKEYBOARD_H__
#define USBKEYBOARD_H__

//==============================
// Typedef USB keyboard struct.
//==============================
typedef struct usbKeyMsg_struct  usbKeyMsg_t;
//==============================
// USB keyboard struct.
//==============================
struct usbKeyMsg_struct {
	uint8_t keypressed;
	uint8_t keyCode;
	uint8_t modifiers;
	uint8_t keyOEM;
	uint8_t repeatchar;
	uint8_t repeatkeyCode;
	uint8_t	repeatmodifiers;
	uint8_t repeatkeyOEM;
	uint16_t last_keyCode;
	uint32_t keyRepeatDelay;
	uint32_t keyRepeatRate;
};

//=================================
// C++ prototypes for USB Keyboard.
//=================================
void OnPress(int key);
void OnRelease(int key);
int USBKeyboard_available(void);
int USBScancode_available(void);
int USBKeyboard_read(void);
uint8_t USBGetScanCode(void);
void checkKeyRepeat(void);
void writeBuff(void);
void USBKeyboardInit(void);
uint16_t USBKeyboard_readscancode(void);
uint16_t USBGetkey(void);
uint8_t USBGetModifiers(void);
uint8_t USBGetOemKey(void);
#endif
