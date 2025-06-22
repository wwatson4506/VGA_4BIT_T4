//============================
// window.cpp
//============================
#include "window.h"

uint8_t winChars[] = {32,32,186,186,200,205,188};
vgawin_t windows[MAXWINDOWS+2];

//============================================================
// Returns starting x position of centered string.
//============================================================
uint8_t getCenteredTextX(uint8_t xwidth, const char * str) {
	return (xwidth / 2) - (strlen(str) / 2);
}

//============================================================
// Draw a simple desk top.
// bgColor:    background color.
// frameColor: color of outline frame.
// titleColor: color of title in title bar.
// deskTitle:  desk top title (centered).
//============================================================
int desktop(uint8_t bgColor, uint8_t frameType, uint8_t frameColor, uint8_t pattern, uint8_t titleColor, const char *deskTitle) {
  uint8_t tempFGC = vga4bit.getFGC();
  uint8_t tempBGC = vga4bit.getBGC();
  uint8_t centerText = getCenteredTextX(vga4bit.getTwidth(),deskTitle);  

  box_color(0,0,2,vga4bit.getTwidth()-1, bgColor);
  vga4bit.setForegroundColor(bgColor);
  box_charfill(2, 0, vga4bit.getTheight(), vga4bit.getTwidth()-1, pattern);
  vga4bit.setBackgroundColor(bgColor);
  vga4bit.setForegroundColor(frameColor);
  box_draw(0 , 0, vga4bit.getTheight()-1, vga4bit.getTwidth()-1, frameType);
  box_draw(0 , 0, 2, vga4bit.getTwidth()-1, frameType);

  vga4bit.textxy(0,2);
  (frameType == SINGLE_LINE) ? vga4bit.write(195) : vga4bit.write(204);
  vga4bit.textxy(vga4bit.getTwidth()-1,2);
  (frameType == SINGLE_LINE) ? vga4bit.write(180) : vga4bit.write(185);

  vga4bit.textxy(centerText,1);
  vga4bit.setForegroundColor(titleColor);
  vga4bit.printf("%s",deskTitle);
  vga4bit.setBackgroundColor(tempBGC);
  vga4bit.setForegroundColor(tempFGC);
  return 0;
}

//============================================================
// Change title in title bar of selected window.
//============================================================
int titleWindow(int winNum, uint8_t titleColor, const char *title) {
  uint8_t centerText = getCenteredTextX((windows[winNum].x2-1)-(windows[winNum].x1+1),title);  
  uint8_t tmpColor = vga4bit.getFGC(); // Save foreground color.
  int16_t tmpx = vga4bit.getTextX(); //Save print position.
  int16_t tmpy = vga4bit.getTextY();
  
  if(windows[winNum].handle > 0) return -1; // Error: Window is active.
  windows[winNum].winTitle = title;	// Update window struct title.
  windows[winNum].titleColor = titleColor; // Update window struct color.
  vga4bit.setForegroundColor(titleColor); // Set title color.
  vga4bit.unsetPrintWindow(); // Temporarily release window boundries.
  // Erase old title and print new title.
  box_erase(windows[winNum].y1+1, windows[winNum].x1+1, windows[winNum].y1+1, windows[winNum].x2);
  vga4bit.textxy((windows[winNum].x1+1)+centerText,windows[winNum].y1+1);
  vga4bit.printf("%s",windows[winNum].winTitle);
  vga4bit.textxy(tmpx,tmpy); // Restore print position.
  vga4bit.setForegroundColor(tmpColor); // Restore foreground color.
  selectWindow(winNum); // Restore original window boundries.
  return 0;
}

//============================================================
// Draw window given x1, y1, x2, y2 rectangle in current page.
//============================================================
int openWindow(int winNum) {
  uint8_t centerText = getCenteredTextX((windows[winNum].x2-1)-(windows[winNum].x1+1),windows[winNum].winTitle);  
  uint8_t tempBGC = 0;
  
  if(windows[winNum].handle >= 0) return -1; // Error: Window is active.

  windows[winNum].handle = winNum;
  windows[winNum].winbuf = vbox_get(windows[winNum].y1,windows[winNum].x1,
                                    windows[winNum].y2+4,windows[winNum].x2+2);

  vga4bit.setBackgroundColor(windows[winNum].bgc);
  box_erase(windows[winNum].y1,windows[winNum].x1,
            windows[winNum].y2+3,windows[winNum].x2+1);
  
  vga4bit.setForegroundColor(windows[winNum].frameColor);
  box_draw(windows[winNum].y1,windows[winNum].x1,windows[winNum].y2+3,
           windows[winNum].x2+1,windows[winNum].frameType);
  box_draw(windows[winNum].y1,windows[winNum].x1,windows[winNum].y1+2,
           windows[winNum].x2+1,windows[winNum].frameType);

  vga4bit.textxy(windows[winNum].x1,windows[winNum].y1+2);
  (windows[winNum].frameType == SINGLE_LINE) ? vga4bit.write(195) : vga4bit.write(204);
  vga4bit.textxy(windows[winNum].x2+1,windows[winNum].y1+2);
  (windows[winNum].frameType == SINGLE_LINE) ? vga4bit.write(180) : vga4bit.write(185);

  if(windows[winNum].shadowEnable) {
    tempBGC = vga4bit.getBGC();
    vga4bit.setBackgroundColor(windows[winNum].shadowBGColor);
    vga4bit.setForegroundColor(windows[winNum].shadowColor);
    box_charfill(windows[winNum].y2+4,windows[winNum].x1+1,windows[winNum].y2+4,
              windows[winNum].x2+1, windows[winNum].shadowPattern);
    box_charfill(windows[winNum].y1+1,windows[winNum].x2+2,windows[winNum].y2+4,
              windows[winNum].x2+2, windows[winNum].shadowPattern);
    vga4bit.setBackgroundColor(tempBGC);
  }
  
  vga4bit.setForegroundColor(windows[winNum].titleColor);
  vga4bit.textxy((windows[winNum].x1+1)+centerText,windows[winNum].y1+1);
  vga4bit.printf("%s",windows[winNum].winTitle);

  vga4bit.setForegroundColor(windows[winNum].fgc);
  vga4bit.setBackgroundColor(windows[winNum].bgc);

  windows[winNum].isOpen = true;

  return windows[winNum].handle; // Return handle.
}

//============================================================
// Get address of window structure array.
//============================================================
vgawin_t *getWinPointer(void) {
  return windows;	
}

//============================================================
// Only one active window at a time. Return active window.
//============================================================
int getActiveWin(void) {
  for(int i = 0; i <= MAXWINDOWS; i++) {
    if(windows[i].active == true) return i;
  }
  return 0; // Default to 0. No open windows.
}

//================================================================
// Select an open window. Return -1 if not open or active window.
//================================================================
int selectWindow(int winNum) {
  vga4bit.setForegroundColor(windows[winNum].fgc);
  vga4bit.setBackgroundColor(windows[winNum].bgc);

  int actv = getActiveWin();
  windows[actv].active = false;
  windows[winNum].active = true;
  vga4bit.setPrintCWindow(windows[winNum].x1+1,windows[winNum].y1+3, //+4,
                          windows[winNum].x2-windows[winNum].x1,
                          windows[winNum].y2-windows[winNum].y1);
  windows[winNum].active = true;

  return windows[winNum].handle; // Return active handle.
}

//============================================================
// Close an open window
//============================================================
int windowClose(int winNum) {
  if(windows[winNum].handle < 0) return -1; // Error: window is not open.
  vbox_put(windows[winNum].winbuf);
  windows[winNum].handle = -1;
  windows[winNum].active = false;
  windows[winNum].isOpen = false;
  free(windows[winNum].winbuf);
  vga4bit.unsetPrintWindow();
  return 0;
}

//============================================================
// Clear a window and home cursor.
//============================================================
int clearWindow(int winNum) {
  int actv = getActiveWin();
  selectWindow(winNum);
  box_erase(0, 0, windows[winNum].y2, windows[winNum].x2);
  vga4bit.textxy(0,0); // Home cursor.
  selectWindow(actv);
  return 0;
}
