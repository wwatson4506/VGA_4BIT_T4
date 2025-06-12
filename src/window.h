//============================
// window.h
//============================
#ifndef _WINDOW_H
#define _WINDOW_H

#include "VGA_4bit_T4.h"
#include "box.h"

#define MAXWINDOWS   10
#define SINGLE_LINE  1
#define DOUBLE_LINE  2
       
typedef struct {
  int handle = -1;
  bool isOpen = false;
  bool active = false;
  uint8_t x1;
  uint8_t y1;
  uint8_t x2;
  uint8_t y2;
  uint8_t fgc;
  uint8_t bgc;
  uint8_t fgcSave;
  uint8_t bgcSave;
  uint8_t frameColor;
  uint8_t frameType;
  bool shadowEnable = false;
  uint8_t shadowColor;
  uint8_t titleColor;
  const char *winTitle;
  uint8_t *winbuf = NULL;
} vgawin_t;

uint8_t getCenteredTextX(uint8_t xwidth, const char * str);
int desktop(uint8_t bgColor, uint8_t frameType, uint8_t frameColor,uint8_t titleColor, const char *deskTitle);
int titleWindow(int winNum, uint8_t titleColor, const char *title);
int openWindow(int winNum);
vgawin_t *getWinPointer(void);
int getActiveWin(void);
int selectWindow(int winNum);
int clearWindow(int winNum);
int windowClose(int winNum);

#endif // _WINDOW_H
