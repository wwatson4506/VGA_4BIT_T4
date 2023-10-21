/*
   This file is part of VGA_4bit_T4 library.

   VGA_4bit_T4 library is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   VGA_4bit_T4 library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with VGA_4bit_T4 library. If not, see <http://www.gnu.org/licenses/>.

   Copyright (C) 2023 Warren Watson

   Original Author: Watson <wwatson4506@gmail.com>

  This is a modified version of a 4 bit VGA driver using flexio by jmarsh.
  https://forum.pjrc.com/threads/72710-VGA-output-via-FlexIO-(Teensy4).
  Various graphic and text functions taken from uVGA by Eric PREVOTEAU
  and adapted for use with VGA_4bit_T4.
  https://github.com/qix67/uVGA.
*/

#ifndef _VGA_4BIT_T4_H
#define _VGA_4BIT_T4_H

#include "Arduino.h"
#include <DMAChannel.h>
#include "VGA_T4_Config.h"

/* R2R ladder:
 *
 * GROUND <------------- 536R ----*---- 270R -----*---------> VGA PIN: Red=1
 *                                |               |
 * INTENSITY (13) <---536R -------/               |
 *                                                |
 * T4-11 <---536R --------------------------------/
 *
 * GROUND <------------- 536R ----*---- 270R -----*---------> VGA PIN: Green=2
 *                                |               |
 * INTENSITY (13) <---536R -------/               |
 *                                                |
 * T4-12 <---536R --------------------------------/
 *
 * GROUND <------------- 536R ----*---- 270R -----*---------> VGA PIN: Blue=3
 *                                |               |
 * INTENSITY (13) <---536R -------/               |
 *                                                |
 * T4-10 <---536R --------------------------------/
 *
 * VSYNC (T34) <---------------68R---------------------------> VGA PIN 14
 *
 * HSYNC (T35) <---------------68R---------------------------> VGA PIN 13
 */

// Color defines for 4 bit VGA.
#define VGA_BLACK  0
#define VGA_BLUE    1
#define VGA_GREEN   2
#define VGA_CYAN    3
#define VGA_RED     4
#define VGA_MAGENTA 5
#define VGA_YELLOW  6
#define VGA_WHITE   7
#define VGA_GREY    8
#define VGA_BRIGHT_BLUE    9
#define VGA_BRIGHT_GREEN   10
#define VGA_BRIGHT_CYAN    11
#define VGA_BRIGHT_RED     12
#define VGA_BRIGHT_MAGENTA 13 // MAGENTA ?
#define VGA_BRIGHT_YELLOW  14
#define VGA_BRIGHT_WHITE   15

#define GET_L_NIBBLE(l) l >> 4 & 0x0f
#define GET_R_NIBBLE(r) r & 0x0f

typedef enum vga_text_direction
{
  VGA_DIR_RIGHT,
  VGA_DIR_TOP,
  VGA_DIR_LEFT,
  VGA_DIR_BOTTOM,
} vga_text_direction;

//**************************************************************//
// Graphic Cursor Image Array
//**************************************************************//
const uint8_t arrow[][128] = {
 {}, // Image 0 reserved for block cursor.

// Graphic Cursor Image 1 (Solid White Arrow)
//**************************************************************//
 { 0X0f,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X0f,0X0f,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X0f,0X00,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X0f,0X0f,0X00,0X00,0X00,
   0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,0X00,
   0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,0X00
 },

// Graphic Cursor Image 2 (Hollow Arrow) 
//**************************************************************//
  {
   0X0f,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X0f,0X00,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X0f,0X00,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X00,0X0f,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X00,0X0f,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,
   0X0f,0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,
   0X0f,0X00,0X00,0X00,0X0f,0X0f,0X00,0X00,
   0X0f,0X00,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X0f,0X00,0X00,0X0f,0X0f,0X00,0X00,0X00,
   0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,0X00,
   0X00,0X00,0X00,0X00,0X0f,0X0f,0X00,0X00
  },

// Graphic Cursor Image 3 (I-beam) 
//**************************************************************//
  {
   0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X00,0X00,0X0f,0X00,0X00,0X00,0X00,0X00,
   0X0f,0X0f,0X0f,0X0f,0X0f,0X00,0X00,0X00,
   0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00
  }
};

// Text cursor struct.
typedef struct {
  bool     active = false;  // Cursor on/off.
  int16_t tCursor_x;       // Pixel based.
  int16_t tCursor_y;       // Pixel based.
  uint8_t  char_under_cursor[256]; // Char save array.
  bool     blink;           // Blink enable. True = blink.
  uint32_t blink_rate;	
  bool     toggle;          // Cursor blink togggle.
  uint8_t  x_start;         // Cursor x start (0-7).
  uint8_t  y_start;         // Cursor y start (0-15).
  uint8_t  x_end;           // Cursor y end (0-7).
  uint8_t  y_end;           // Cursor y end (0-15).
} text_cursor;

// Graphic cursor struct.
typedef struct {
  bool     active = false;  // gCursor on/off.
  int16_t gCursor_x;       // Pixel based.
  int16_t gCursor_y;       // Pixel based.
  uint8_t  char_under_cursor[256]; // Char save array.
  uint8_t  x_start;         // Cursor x start (0-7).
  uint8_t  y_start;         // Cursor y start (0-15).
  uint8_t  x_end;           // Cursor y end (0-7).
  uint8_t  y_end;           // Cursor y end (0-15).
  uint8_t  type;            // Graphic type. 0=block, 1=arrow, 2=hollow arrow.
} graphic_cursor;

// Cursor types
#define BLOCK_CURSOR 0
#define FILLED_ARROW 1
#define HOLLOW_ARROW 2
#define I_BEAM       3

//***************************************************************
// horizontal values must be divisible by 8 for correct operation
//***************************************************************
typedef struct {
  bool     active = false;
  uint32_t height;
  uint32_t vfp;
  uint32_t vsw;
  uint32_t vbp;
  uint32_t width;
  uint32_t hfp;
  uint32_t hsw;
  uint32_t hbp;
  uint32_t clk_num;
  uint32_t clk_den;
  // sync polarities: 0 = active high, 1 = active low
  uint32_t vsync_pol;
  uint32_t hsync_pol;
} vga_timing;

//********************************************************
// Timing modes. 1024x600x60 is not right yet...
//********************************************************
PROGMEM static const vga_timing t1280x720x60 = {
  .height=720, .vfp=13, .vsw=5, .vbp=12,
  .width=1280, .hfp=80, .hsw=40, .hbp=248,
  .clk_num=7425, .clk_den=2400, .vsync_pol=0, .hsync_pol=0
};

PROGMEM static const vga_timing t1024x768x60 = {
  .height=768, .vfp=3, .vsw=6, .vbp=29,
  .width=1024, .hfp=24, .hsw=136, .hbp=160,
  .clk_num=65, .clk_den=24, .vsync_pol=1, .hsync_pol=1
};

// Added
PROGMEM static const vga_timing t1024x600x60 = {
  .height=600, .vfp=1, .vsw=4, .vbp=23,
  .width=1024, .hfp=56, .hsw=160, .hbp=112,
  .clk_num=32, .clk_den=15, .vsync_pol=0, .hsync_pol=1
};

PROGMEM static const vga_timing t800x600x100 = {
  .height=600, .vfp=1, .vsw=3, .vbp=32,
  .width=800, .hfp=48, .hsw=88, .hbp=136,
  .clk_num=6818, .clk_den=2400, .vsync_pol=0, .hsync_pol=1
};

PROGMEM static const vga_timing t800x600x60 = {
  .height=600, .vfp=1, .vsw=4, .vbp=23,
  .width=800, .hfp=40, .hsw=128, .hbp=88,
  .clk_num=40, .clk_den=24, .vsync_pol=0, .hsync_pol=0
};

PROGMEM static const vga_timing t640x480x60 = {
  .height=480, .vfp=10, .vsw=2, .vbp=33,
  .width=640, .hfp=16, .hsw=96, .hbp=48,
  .clk_num=150, .clk_den=143, .vsync_pol=1, .hsync_pol=1
};

PROGMEM static const vga_timing t640x400x70 = {
  .height=400, .vfp=12, .vsw=2, .vbp=35,
  .width=640, .hfp=16, .hsw=96, .hbp=48,
  .clk_num=150, .clk_den=143, .vsync_pol=0, .hsync_pol=1
};

PROGMEM static const vga_timing t640x350x70 = {
  .height=350, .vfp=37, .vsw=2, .vbp=60,
  .width=640, .hfp=16, .hsw=96, .hbp=48,
  .clk_num=150, .clk_den=143, .vsync_pol=1, .hsync_pol=0
};

//***************************************************************
#define STRIDE_PADDING 16
#define SWAP(x,y) { (x)=(x)^(y); (y)=(x)^(y); (x)=(x)^(y); }
//***************************************************************
// FlexIO2VGA class
//***************************************************************
class FlexIO2VGA : public Print
{
public:

  FlexIO2VGA() {};
  
  void begin(const vga_timing& mode, bool half_height=false,
             bool half_width=false, unsigned int bpp=4);
  void stop(void);

  // wait parameter:
  // TRUE =  wait until previous frame ends and source is "current"
  // FALSE = queue it as the next framebuffer to draw, return immediately
  void set_next_buffer(const void* source, size_t pitch, bool wait);

  void wait_for_frame(void) {
    unsigned int count = frameCount;
    while (count == frameCount) yield();
  }

  void init();  // Currently unused

  // Frame buffer and init methods
  void fbUpdate(bool wait);
  uint8_t *getFB(void) { return _fb; } // Return a pointer to the active
                                       // frame buffer
  size_t getPitch(void); // return stride size
  void setDoubleWidth(bool doubleWidth);  
  void setDoubleHeight(bool doubleHeight);  
  void getFbSize(int *width, int *height);
  
  uint16_t getGwidth(void);
  uint16_t getGheight(void);
  uint16_t getTwidth(void);
  uint16_t getTheight(void);
  uint8_t  getFGC(void) { return foreground_color; }
  uint8_t  getBGC(void) { return background_color; }

  // Initialize text settings
  void init_text_settings();

  // Software text cursor methods
  void initCursor(uint8_t xStart, uint8_t yStart, uint8_t xEnd,
                  uint8_t yEnd, bool blink, uint32_t blink_rate);	
  void cursorOn(void);
  void cursorOff(void);
  uint16_t tCursorX(void);
  uint16_t tCursorY(void);
  void drawCursor(int color);
  void updateTCursor(int column, int line);
  void setCursorBlink(bool onOff);
  void setCursorType(uint8_t cursorType);
  
  // Software graphic cursor methods
  void initGcursor(uint8_t type, uint8_t xStart, uint8_t yStart,
                   uint8_t xEnd, uint8_t yEnd);
  void gCursorOn(void);
  void gCursorOff(void);
  uint16_t gCursorX(void);
  uint16_t gCursorY(void);
  void drawGcursor(int color);
  void moveGcursor(int16_t column, int16_t line);

  // low level frame buffer methods
  uint8_t getByte(uint32_t x, uint32_t y);
  void getChar(int16_t x, int16_t y, uint8_t *buf);
  void getGptr(int16_t x, int16_t y, uint8_t *buf);
  void putByte(uint32_t x, uint32_t y, uint8_t byte);
  void putChar(int16_t x, int16_t y, uint8_t *buf);
  void putGptr(int16_t x, int16_t y, uint8_t *buf);
  void writeVmem(uint8_t *buf, uint32_t vMem, uint32_t size);
  void readVmem(uint32_t vMem, uint8_t *buf, int32_t size);

  // Graphic methods
  void drawPixel(int16_t x, int16_t y, uint8_t fg);
  uint8_t getPixel(uint32_t x, uint32_t y);
  void drawHLine(int y, int x1, int x2, int color);
  void drawVLine(int y, int x1, int x2, int color);
  // delta = abs(v2-v1); sign = sign of (v2-v1);
  inline void delta_and_sign(int v1, int v2, int *delta, int *sign);
  void drawLine(int x0, int y0, int x1, int y1, int color, bool no_last_pixel);
  void drawRect(int x0, int y0, int x1, int y1, int color);
  void fillRect(int x0, int y0, int x1, int y1, int color);
  void drawCircle(float x, float y, float radius, float thickness, uint8_t color);
  void fillCircle(float xm, float ym, float r, uint8_t color);
  void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int color);
  void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int color);
  void drawEllipse(int _x0, int _y0, int _x1, int _y1, int color);
  void fillEllipse(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
  void drawBitmap(int16_t x_pos, int16_t y_pos, uint8_t *bitmap, int16_t bitmap_width, int16_t bitmap_height);
  void copy(int s_x, int s_y, int d_x, int d_y, int w, int h);

// Text methods
  void clear(uint8_t fg); // Clear full screen to fg color
  // define text print window. Width and height are in pixels
  // and cannot be smaller than font width and height
  void setPrintWindow(int x, int y, int width, int height);
  void unsetPrintWindow();
  void clearPrintWindow();
  void scrollPrintWindow();
  void scrollUp();
  void scrollDown();
  void scroll(int x, int y, int w, int h, int dx, int dy,int col);
  void drawText(int16_t x, int16_t y, const char * text, uint8_t fgcolor, uint8_t bgcolor);
  void drawText(int16_t x, int16_t y, const char * text, uint8_t fgcolor, uint8_t bgcolor, vga_text_direction dir);
  int  setFontSize(uint8_t fsize, bool runflag);  
  int  getFontWidth(void) { return font_width; }
  int  getFontHeight(void) { return font_height; }
  void setForegroundColor(int8_t fg_color); // RGBI format
  void setBackgroundColor(int8_t bg_color); // RGBI format
  void textxy(int column, int line);
  int16_t getTextX(void) { return cursor_x; }
  int16_t getTextY(void) { return cursor_y; }
  uint8_t getTextBGC(void) { return background_color; }
  uint8_t getTextFGC(void) { return foreground_color; }
  void textColor(uint8_t fgc, uint8_t bgc);
  void setPromptSize(uint16_t ps); 
 
  virtual size_t write(const char *buffer, size_t size);
  virtual size_t write(uint8_t c);

  // Methods used by VT100 terminal and text editors
  void clreol(void);
  void clreos(void);
  void clrbol(void);
  void clrbos(void);
  void clrlin(void);

  void clearStatusLine(uint8_t bgc);
  void slWrite(int16_t x,  uint16_t fgcolor, uint16_t bgcolor, const char * text);

  uint16_t promp_size = 0;
  
private:
  void set_clk(int num, int den);
  static void ISR(void);
  void TimerInterrupt(void);
  
 
  // Inline methods
  inline int clip_x(int x);
  inline int clip_y(int y);
  inline void drawHLineFast(int y, int x1, int x2, int color);
  inline void drawVLineFast(int x, int y1, int y2, int color);
  inline void drawLinex(int x0, int y0, int x1, int y1, int color) {
    drawLine(x0, y0, x1, y1, color, true);
  }
  inline void Vscroll(int x, int y, int w, int h, int dy ,int col);
  inline void Hscroll(int x, int y, int w, int h, int dx ,int col);

  // Private variables
  uint8_t foreground_color;
  uint8_t background_color;
  bool transparent_background;

  short cursor_x;		// cursor x position in print window in CHARACTER
  short cursor_y;		// cursor y position in print window in CHARACTER
  short font_width;		// Font width: 8
  short font_height;	// Font height 8 or 16
  short print_window_x;	// x position in pixel of text window 
  short print_window_y;	// y position in pixel of text window
  short print_window_w;	// text window width in CHARACTER
  short print_window_h;	// text window height in CHARACTER

  uint8_t dma_chans[2];
  DMAChannel dma1,dma2,dmaswitcher;
  DMASetting dma_params;

  bool double_height;
  bool double_width;
  int32_t widthxbpp;
  int bpp;

  uint32_t cursor = 0;
  
  uint8_t *_fb = NULL;
  volatile unsigned int frameCount;
};

extern FlexIO2VGA vga4bit;
#endif
