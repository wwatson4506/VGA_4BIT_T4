//=============================
// USBKeyboard.h
// By: Warren Watson 2017-2023
//=============================
#ifndef USBKEYBOARD_H__
#define USBKEYBOARD_H__

#define KEY_ESCAPE     27
#define MY_KEY_F1     194
#define MY_KEY_F2     195
#define MY_KEY_F3     196
#define MY_KEY_F4     197
#define MY_KEY_F5     198
#define MY_KEY_F6     199
#define MY_KEY_F7     200
#define MY_KEY_F8     201
#define MY_KEY_F9     202
#define MY_KEY_F10    203
#define MY_KEY_F11    204
#define MY_KEY_F12    205
/*
#define KEY_SHIFT_F1   21504
#define KEY_SHIFT_F2   21760
#define KEY_SHIFT_F3   22016
#define KEY_SHIFT_F4   22272
#define KEY_SHIFT_F5   22528
#define KEY_SHIFT_F6   22784
#define KEY_SHIFT_F7   23040
#define KEY_SHIFT_F8   23296
#define KEY_SHIFT_F9   23552
#define KEY_SHIFT_F10  23808

#define KEY_CTRL_F1    24064
#define KEY_CTRL_F2    24320
#define KEY_CTRL_F3    24576
#define KEY_CTRL_F4    24832
#define KEY_CTRL_F5    25088
#define KEY_CTRL_F6    25344
#define KEY_CTRL_F7    25600
#define KEY_CTRL_F8    25856
#define KEY_CTRL_F9    26112
#define KEY_CTRL_F10   26368

#define KEY_ALT_F1     26624
#define KEY_ALT_F2     26880
#define KEY_ALT_F3     27136
#define KEY_ALT_F4     27392
#define KEY_ALT_F5     27648
#define KEY_ALT_F6     27904
#define KEY_ALT_F7     28160
#define KEY_ALT_F8     28416
#define KEY_ALT_F9     28672
#define KEY_ALT_F10    28928
*/
#define MY_KEY_INSERT  209
#define MY_KEY_HOME    210
#define MY_KEY_PGUP    211
#define MY_KEY_DELETE  212
#define MY_KEY_END     213
#define MY_KEY_PGDN    214

#define MY_KEY_UP        218
#define MY_KEY_LEFT      216
#define MY_KEY_DOWN      217
#define MY_KEY_RIGHT     215
#define MY_KEY_ENTER     10
#define MY_KEY_ESCAPE    27
#define MY_KEY_BACKSPACE 8
#define MY_KEY_TAB       9
/*
#define KEY_SHIFT_TAB  3840
#define KEY_CTRL_LEFT  29440
#define KEY_CTRL_RIGHT 29696
#define KEY_CTRL_HOME  30464
#define KEY_CTRL_PGUP  33792
#define KEY_CTRL_PGDN  30208
#define KEY_CTRL_END   29952
#define KEY_CTRL_ENTER 10
*/

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
uint8_t getSCorKP(bool scORkp);
char *cgets(char *s);
#endif
