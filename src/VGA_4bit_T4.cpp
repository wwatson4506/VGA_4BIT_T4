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

#include "VGA_4bit_T4.h"
#include "VGA_T4_Config.h"
#include "font_8x8.h"
#include "font_8x16.h"

//==============================================
// Original version of the 4 bit VGA DAC ladder.
//==============================================
/* R2R ladder:
 *
 * GROUND <------------- 536R ----*---- 270R -----*---------> VGA PIN: Red pin 1
 *                                |               |
 * INTENSITY (13) <---536R -------/               |
 *                                                |
 * T4-11 <---536R --------------------------------/
 *
 * GROUND <------------- 536R ----*---- 270R -----*---------> VGA PIN: Green pin 2
 *                                |               |
 * INTENSITY (13) <---536R -------/               |
 *                                                |
 * T4-12 <---536R --------------------------------/
 *
 * GROUND <------------- 536R ----*---- 270R -----*---------> VGA PIN: Blue pin3
 *                                |               |
 * INTENSITY (13) <---536R -------/               |
 *                                                |
 * T4-10 <---536R --------------------------------/
 *
 * VSYNC (34) <---------------68R---------------------------> VGA PIN 14 - T4 pin 34
 *
 * HSYNC (35) <---------------68R---------------------------> VGA PIN 13 - T4 pin 35
 */

DMAMEM uint8_t frameBuffer0[(MAX_HEIGHT+1)*(MAX_WIDTH+STRIDE_PADDING)];
//uint8_t frameBuffer0[(MAX_HEIGHT+1)*(MAX_WIDTH+STRIDE_PADDING)];
//EXTMEM uint8_t frameBuffer0[(MAX_HEIGHT+1)*(MAX_WIDTH+STRIDE_PADDING)];
//static uint8_t frameBuffer1[(MAX_HEIGHT+1)*(MAX_WIDTH+STRIDE_PADDING)];
static uint8_t* const s_frameBuffer[1] = {frameBuffer0};

static uint32_t frameBufferIndex = 0;
static int fb_width;
static int fb_height;
static size_t _pitch;  
text_cursor tCursor;
graphic_cursor gCursor;

//===============================================
// Start 4 bit VGA display.
// Params:
//  mode - pointer to vga_timing struct. One of:
//           t640x400x70; -- needs tweaking --
//           t640x480x60;
//           t800x600x60;
//           t1024x600x60 -- needs tweaking --
//           t1024x768x60
//  bool half_ height - Screen height doubling.
//  bool half_width.  - Screen width doubling.
//  bpp: Bits per pixel 1 or 4.
//         (only 4 bit pixels spported at this time) 
//===============================================
FLASHMEM void FlexIO2VGA::begin(const vga_timing& mode, bool half_height, bool half_width, unsigned int bpp) {
  frameCount = 0;
  *(portConfigRegister(11)) = 4; // FLEXIO2_D2    RED
  *(portConfigRegister(12)) = 4; // FLEXIO2_D1    GREEN
  *(portConfigRegister(10)) = 4; // FLEXIO2_D0    BLUE
  *(portConfigRegister(13)) = 4; // FLEXIO2_D3    INTENSITY
  *(portConfigRegister(34)) = 4; // FLEXIO2_D29   VSYNC
  *(portConfigRegister(35)) = 4; // FLEXIO2_D30   HSYNC

  dma_chans[0] = dma2.channel;
  dma_chans[1] = dma1.channel;

  memset(dma_params.TCD, 0, sizeof(*dma_params.TCD));
  dma_params.TCD->DOFF = 4;
  dma_params.TCD->ATTR = DMA_TCD_ATTR_DMOD(3) | DMA_TCD_ATTR_DSIZE(2);
  dma_params.TCD->NBYTES = 8;
  dma_params.TCD->DADDR = &FLEXIO2_SHIFTBUF0;
  dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_FLEXIO2_REQUEST0);
  dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_FLEXIO2_REQUEST0);

  dmaswitcher.TCD->SADDR = dma_chans;
  dmaswitcher.TCD->SOFF = 1;
  dmaswitcher.TCD->DADDR = &DMA_SERQ;
  dmaswitcher.TCD->DOFF = 0;
  dmaswitcher.TCD->ATTR = DMA_TCD_ATTR_SMOD(1);
  dmaswitcher.TCD->NBYTES = 1;
  dmaswitcher.TCD->BITER = dmaswitcher.TCD->CITER = 1;

  double_width = half_width;   // This was missing, added.
  double_height = half_height;
  widthxbpp = (mode.width * bpp) / (half_width ? 2 : 1);
  set_clk(4*mode.clk_num, mode.clk_den);

  fb_width = mode.width / (double_width ? 2:1);
  fb_height = mode.height / (double_height ? 2:1);
  _pitch = fb_width*bpp / 8 + STRIDE_PADDING;

  FLEXIO2_CTRL = FLEXIO_CTRL_SWRST;
  asm volatile("dsb");
  FLEXIO2_CTRL = FLEXIO_CTRL_FASTACC | FLEXIO_CTRL_FLEXEN;
  // wait for reset to clear
  while (FLEXIO2_CTRL & FLEXIO_CTRL_SWRST);

  // timer 0: divide pixel clock by 8
  FLEXIO2_TIMCFG0 = 0;
  FLEXIO2_TIMCMP0 = (4*8)-1;
  
  // timer 1: generate HSYNC
  FLEXIO2_TIMCFG1 = FLEXIO_TIMCFG_TIMDEC(1);
  // on = HSW, off = rest of line
  FLEXIO2_TIMCMP1 = ((((mode.width+mode.hbp+mode.hfp)/8)-1)<<8) | ((mode.hsw/8)-1);
  // trigger = timer0, HSYNC=D28
  FLEXIO2_TIMCTL1 = FLEXIO_TIMCTL_TRGSEL(4*0+3) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(28) | FLEXIO_TIMCTL_TIMOD(2) | (mode.hsync_pol*FLEXIO_TIMCTL_PINPOL);

  // timer 2: frame counter
  // tick on HSYNC
  FLEXIO2_TIMCFG2 = FLEXIO_TIMCFG_TIMDEC(1);
  FLEXIO2_TIMCMP2 = ((mode.height+mode.vbp+mode.vfp+mode.vsw)*2)-1;
  // trigger = HYSNC pin
  FLEXIO2_TIMCTL2 = FLEXIO_TIMCTL_TRGSEL(2*28) | (mode.hsync_pol * FLEXIO_TIMCTL_TRGPOL) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TIMOD(3);

  // timer 3: generate VSYNC
  FLEXIO2_TIMCFG3 = FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(7);
  // active for VSW lines. 4*total horizontal pixels*vertical sync loength must be <= 65536 to not overflow this timer
  FLEXIO2_TIMCMP3 = (4*mode.vsw*(mode.width+mode.hbp+mode.hsw+mode.hfp))-1;
  // trigger = frame counter, VSYNC=D29
  FLEXIO2_TIMCTL3 = FLEXIO_TIMCTL_TRGSEL(4*2+3) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(29) | FLEXIO_TIMCTL_TIMOD(3) | (mode.vsync_pol*FLEXIO_TIMCTL_PINPOL);

  // timer4: count VSYNC and back porch
  // enable on VSYNC start, disable after (VSW+VBP)*2 edges of HSYNC
  FLEXIO2_TIMCFG4 = FLEXIO_TIMCFG_TIMDEC(2) | FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(6);
  FLEXIO2_TIMCMP4 = ((mode.vsw+mode.vbp)*2)-1;
  // trigger = VSYNC pin, pin = HSYNC
  FLEXIO2_TIMCTL4 = FLEXIO_TIMCTL_TRGSEL(2*29) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_PINSEL(28) | FLEXIO_TIMCTL_TIMOD(3) | (mode.vsync_pol*FLEXIO_TIMCTL_TRGPOL) | (mode.hsync_pol*FLEXIO_TIMCTL_PINPOL);

  // timer 5: vertical active region
  // enable when previous timer finishes, disable after height*2 edges of HSYNC
  FLEXIO2_TIMCFG5 = FLEXIO_TIMCFG_TIMDEC(2) | FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(6);
  FLEXIO2_TIMCMP5 = (mode.height*2)-1;
  // trigger = timer4 negative, pin = HSYNC
  FLEXIO2_TIMCTL5 = FLEXIO_TIMCTL_TRGSEL(4*4+3) | FLEXIO_TIMCTL_TRGPOL | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_PINSEL(28) | FLEXIO_TIMCTL_TIMOD(3) | (mode.vsync_pol*FLEXIO_TIMCTL_PINPOL);

  // timer 6: horizontal active region
  // configured as PWM: OFF for HSYNC+HBP, ON for active region, reset (to off state) when HSYNC occurs (off state covers HFP then resets)
  FLEXIO2_TIMCFG6 = FLEXIO_TIMCFG_TIMOUT(1) | FLEXIO_TIMCFG_TIMDEC(1) | FLEXIO_TIMCFG_TIMRST(4) | FLEXIO_TIMCFG_TIMDIS(1) | FLEXIO_TIMCFG_TIMENA(1);
  FLEXIO2_TIMCMP6 = ((((mode.hsw+mode.hbp)/8)-1)<<8) | ((mode.width/8)-1);
  // trigger = timer0, pin = HSYNC
  FLEXIO2_TIMCTL6 = FLEXIO_TIMCTL_TRGSEL(4*0+3) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_PINSEL(28) | FLEXIO_TIMCTL_TIMOD(2) | (mode.hsync_pol*FLEXIO_TIMCTL_PINPOL);

  // timer 7: output pixels from shifter, runs only when trigger is ON
  FLEXIO2_TIMCFG7 = FLEXIO_TIMCFG_TIMDIS(6) | FLEXIO_TIMCFG_TIMENA(6) | FLEXIO_TIMCFG_TSTOP(2);
  FLEXIO2_TIMCMP7 = ((((64/bpp)*2)-1)<<8) | ((half_width ? 4:2)-1);
  // trigger = timer 6
  FLEXIO2_TIMCTL7 = FLEXIO_TIMCTL_TRGSEL(4*6+3) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TIMOD(1);

  // start blank
  FLEXIO2_SHIFTBUF1 = FLEXIO2_SHIFTBUF2 = 0;
  if (bpp == 4) {
    FLEXIO2_SHIFTCFG1 = FLEXIO_SHIFTCFG_PWIDTH(3);
    FLEXIO2_SHIFTCTL1 = FLEXIO_SHIFTCTL_TIMSEL(7) | FLEXIO_SHIFTCTL_SMOD(2);
    // output stop bit when timer disables - ensures black output/zero outside active window
    FLEXIO2_SHIFTCFG0 = FLEXIO_SHIFTCFG_PWIDTH(3) | FLEXIO_SHIFTCFG_INSRC | FLEXIO_SHIFTCFG_SSTOP(2);
    FLEXIO2_SHIFTCTL0 = FLEXIO_SHIFTCTL_TIMSEL(7) | FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(0) | FLEXIO_SHIFTCTL_SMOD(2);
    
    FLEXIO2_SHIFTSTATE = 0;
  } else { // bpp==1
    FLEXIO2_SHIFTCFG1 = FLEXIO_SHIFTCFG_PWIDTH(0);
    FLEXIO2_SHIFTCTL1 = FLEXIO_SHIFTCTL_TIMSEL(7) | FLEXIO_SHIFTCTL_SMOD(2);
    FLEXIO2_SHIFTCFG0 = FLEXIO_SHIFTCFG_PWIDTH(0) | FLEXIO_SHIFTCFG_INSRC | FLEXIO_SHIFTCFG_SSTOP(2);
    FLEXIO2_SHIFTCTL0 = FLEXIO_SHIFTCTL_TIMSEL(7)  | FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(8) | FLEXIO_SHIFTCTL_SMOD(2);

    // D8 clear = use state 2, D8 set = use state 3
    // note that PWIDTH does not seem to mask D4-7 outputs as documented!
    FLEXIO2_SHIFTBUF2 = 0x0069A69A;
    FLEXIO2_SHIFTCFG2 = FLEXIO_SHIFTCFG_PWIDTH(15);
    FLEXIO2_SHIFTCTL2 = FLEXIO_SHIFTCTL_TIMSEL(7) | FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(8) | FLEXIO_SHIFTCTL_SMOD(6) | FLEXIO_SHIFTCTL_TIMPOL;
    FLEXIO2_SHIFTBUF3 = 0x0F69A69A;
    FLEXIO2_SHIFTCFG3 = FLEXIO_SHIFTCFG_PWIDTH(15);
    FLEXIO2_SHIFTCTL3 = FLEXIO_SHIFTCTL_TIMSEL(7) | FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(8) | FLEXIO_SHIFTCTL_SMOD(6) | FLEXIO_SHIFTCTL_TIMPOL;

    FLEXIO2_SHIFTSTATE = 2;
  }

  // clear timer 5 status
  FLEXIO2_TIMSTAT = 1<<5;
  // make sure no other FlexIO interrupts are enabled
  FLEXIO2_SHIFTSIEN = 0;
  FLEXIO2_SHIFTEIEN = 0;
  // enable timer 5 interrupt
  FLEXIO2_TIMIEN = 1<<5;

  attachInterruptVector(IRQ_FLEXIO2, ISR);
  NVIC_SET_PRIORITY(IRQ_FLEXIO2, 32);
  NVIC_ENABLE_IRQ(IRQ_FLEXIO2);

  // start everything!
  FLEXIO2_TIMCTL0 = FLEXIO_TIMCTL_TIMOD(3);

  // Initialize text and cursor settings to default.
  init_text_settings();
}

//===================================================
// Stop display. Used for screen size/params  change.
//===================================================
FLASHMEM void FlexIO2VGA::stop(void) {
  NVIC_DISABLE_IRQ(IRQ_FLEXIO2);
  // FlexIO2 registers don't work if they have no clock
  if (CCM_CCGR3 & CCM_CCGR3_FLEXIO2(CCM_CCGR_ON)) {
    FLEXIO2_CTRL &= ~FLEXIO_CTRL_FLEXEN;
    FLEXIO2_TIMIEN = 0;
    FLEXIO2_SHIFTSDEN = 0;
  }
  dma1.disable();
  dma2.disable();
  asm volatile("dsb");
}

//===================================================
// Set display timing timers.
//===================================================
FLASHMEM void FlexIO2VGA::set_clk(int num, int den) {
  int post_divide = 0;
  while (num < 27*den) num <<= 1, ++post_divide;
  int div_select = num / den;
  num -= div_select * den;

  // valid range for div_select: 27-54

  // switch video PLL to bypass, enable, set div_select
  CCM_ANALOG_PLL_VIDEO = CCM_ANALOG_PLL_VIDEO_BYPASS | CCM_ANALOG_PLL_VIDEO_ENABLE | CCM_ANALOG_PLL_VIDEO_DIV_SELECT(div_select);
  // clear misc2 vid post-divider
  CCM_ANALOG_MISC2_CLR = CCM_ANALOG_MISC2_VIDEO_DIV(3);
  switch (post_divide) {
      case 0: // div by 1
        CCM_ANALOG_PLL_VIDEO_SET = CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(2);
        break;
      case 1: // div by 2
        CCM_ANALOG_PLL_VIDEO_SET = CCM_ANALOG_PLL_VIDEO_POST_DIV_SELECT(1);
        break;
      // div by 4
      // case 2: PLL_VIDEO pos_div_select already set to 0
      case 3: // div by 8 (4*2)
        CCM_ANALOG_MISC2_SET = CCM_ANALOG_MISC2_VIDEO_DIV(1);
        break;
      case 4: // div by 16 (4*4)
        CCM_ANALOG_MISC2_SET = CCM_ANALOG_MISC2_VIDEO_DIV(3);
        break;
  }
  CCM_ANALOG_PLL_VIDEO_NUM = num;
  CCM_ANALOG_PLL_VIDEO_DENOM = den;
  // ensure PLL is powered
  CCM_ANALOG_PLL_VIDEO_CLR = CCM_ANALOG_PLL_VIDEO_POWERDOWN;
  // wait for lock
  while (!(CCM_ANALOG_PLL_VIDEO & CCM_ANALOG_PLL_VIDEO_LOCK));
  // deactivate bypass
  CCM_ANALOG_PLL_VIDEO_CLR = CCM_ANALOG_PLL_VIDEO_BYPASS;

  // gate clock
  CCM_CCGR3 &= ~CCM_CCGR3_FLEXIO2(CCM_CCGR_ON);
  // FlexIO2 use vid clock (PLL5)
  uint32_t t = CCM_CSCMR2;
  t &= ~CCM_CSCMR2_FLEXIO2_CLK_SEL(3);
  t |= CCM_CSCMR2_FLEXIO2_CLK_SEL(2);
  CCM_CSCMR2 = t;
  // flex gets 1:1 clock, no dividing
  CCM_CS1CDR &= ~(CCM_CS1CDR_FLEXIO2_CLK_PODF(7) | CCM_CS1CDR_FLEXIO2_CLK_PRED(7));
  asm volatile("dsb");
  // disable clock gate
  CCM_CCGR3 |= CCM_CCGR3_FLEXIO2(CCM_CCGR_ON);
}

//===================================================
// Frame interupt:
//   Update frame buffer and count.
//   Procces text cursor if enabled.
//===================================================
void FlexIO2VGA::TimerInterrupt(void) {
  if (dma_params.TCD->SADDR) {
    dma1 = dma_params;
    if (double_height) {
      dma1.disableOnCompletion();
      dmaswitcher.triggerAtCompletionOf(dma1);

      dma2 = dma_params;
      dma2.disableOnCompletion();
      dmaswitcher.triggerAtCompletionOf(dma2);
    }
    dma1.enable();
    // push first pixels into shiftbuf registers
    dma1.triggerManual();
    FLEXIO2_SHIFTSDEN = 1<<0;
  }
  frameCount++;
  // Proccess Cursor.
  if(tCursor.active) {
    if(tCursor.blink) {
      if(!(frameCount % tCursor.blink_rate)) {
        tCursor.toggle ^= true;
        if(tCursor.toggle) {
          drawCursor(foreground_color);
        } else {
          putChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
        }
      }
    } else {

      drawCursor(foreground_color);
    }
  }
  fbUpdate(false); // Must be false else endless loop occurs.
}

//=====================================================
// Set active display buffer and pitch.
// Bool wait - if true wait for last frame to complete.
//=====================================================
void FlexIO2VGA::set_next_buffer(const void* source, size_t pitch, bool wait) {
  // find worst alignment combo of source and pitch
  size_t log_read;
  switch (((size_t)source | pitch) & 7) {
    case 0: // 8 byte alignment
      log_read = 3;
      break;
    case 2: // 2 byte alignment
    case 6:
      log_read = 1;
      break;
    case 4: // 4 byte alignment
      log_read = 2;
      break;
    default: // 1 byte alignment, this will be slow...
      log_read = 0;
  }
  uint16_t major = (widthxbpp+63)/64;
  dma_params.TCD->SOFF = 1 << log_read;
  dma_params.TCD->ATTR_SRC = log_read;
  dma_params.TCD->SADDR = source;
  dma_params.TCD->SLAST = pitch - (major*8);
  dma_params.TCD->CITER = dma_params.TCD->BITER = major;
  if (wait)
    wait_for_frame();
}

//-----------------------
// Set double width mode.
//-----------------------
FLASHMEM void FlexIO2VGA::setDoubleWidth(bool doubleWidth) {
  double_width = doubleWidth; 
}

//------------------------
// Set double height mode.
//------------------------
FLASHMEM void FlexIO2VGA::setDoubleHeight(bool doubleHeight) {
  double_height = doubleHeight; 
}

//-------------------------------------------
// Get frame buffer size ( width and height).
//-------------------------------------------
FLASHMEM void FlexIO2VGA::getFbSize(int *width, int *height) {
  *width = fb_width;
  *height = fb_height;
}

//------------------------
// Get frame buffer width.
//------------------------
uint16_t FlexIO2VGA::getGwidth(void) { return fb_width; }

//-------------------------
// Get frame buffer height.
//-------------------------
uint16_t FlexIO2VGA::getGheight(void) { return fb_height; }

//------------------------
// Get frame buffer width.
//------------------------
FLASHMEM uint16_t  FlexIO2VGA::getTwidth(void) { return fb_width/font_width; }

//-------------------------
// Get frame buffer height.
//-------------------------
FLASHMEM uint16_t  FlexIO2VGA::getTheight(void) { return fb_height/font_height; }

//----------------------------
// Get display pitch (stride).
//----------------------------
FLASHMEM  size_t FlexIO2VGA::getPitch(void) { return _pitch; }

//===================
// Clear full screen.
//===================
FLASHMEM void FlexIO2VGA::clear(uint8_t fg) {
  _fb = s_frameBuffer[frameBufferIndex];
  uint8_t c = (fg<<4) | fg; 
  for(int x = 0; x < fb_width; x++) {
    for(int y = 0; y < fb_height; y++) { 
      _fb[y*_pitch+x] = c;
    }
  }
  getChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
  getGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
}

//===================================
// Turn cursor on and display cursor.
//===================================
FLASHMEM void FlexIO2VGA::cursorOn(void) {
  tCursor.active = true;	
//  drawCursor(foreground_color);
//  getGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
  getChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
}

//===============================================
// Turn cursor off and kill any displayed cursor.
//===============================================
FLASHMEM void FlexIO2VGA::cursorOff(void) {
  tCursor.active = false;	
//  drawCursor(background_color);
  putChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
}

//===================================
// Turn gCursor on and display cursor.
//===================================
FLASHMEM void FlexIO2VGA::gCursorOn(void) {
  gCursor.active = true;	
  getGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
  drawGcursor(foreground_color);
}

//===============================================
// Turn gCursor off and kill any displayed cursor.
//===============================================
FLASHMEM void FlexIO2VGA::gCursorOff(void) {
  gCursor.active = false;	
  putGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
}

//=============================================
// Initialize text cursor.
// xStart = cursor starting x position.
// yStart = cursor starting y position.
// xEnd   = cursor ending x position.
// yEnd   = cursor ending y position.
// blink  = false for solid non-blinking cursor.
//          true for blinking currsor.
// blink_rate = obvious huh!! default is 30.
//==============================================
FLASHMEM void FlexIO2VGA::initCursor(uint8_t xStart, uint8_t yStart, uint8_t xEnd,
                            uint8_t yEnd, bool blink, uint32_t blink_rate) {	
  tCursor.active = false;
  tCursor.blink = blink;
  tCursor.blink_rate = blink_rate;
  tCursor.x_start = xStart;
  tCursor.x_end   = xEnd;
  tCursor.y_start = yStart;
  tCursor.y_end   = yEnd;
  getChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
}

//=====================================
// Enable/Disable blinking text cursor.
//=====================================
void FlexIO2VGA::setCursorBlink(bool onOff) {
  tCursor.blink = onOff;
}

//=====================================================
// Set graphic cursor type:
//  0 = Block, 1 = Arrow, 2 = Hollow arrow, 2 = I-beam.
//=====================================================
void FlexIO2VGA::setCursorType(uint8_t cursorType) {
  gCursor.type = cursorType;
  getGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
}

//=============================================
// Initialize graphic cursor.
// xStart = cursor starting x position.
// yStart = cursor starting y position.
// xEnd   = cursor ending x position.
// yEnd   = cursor ending y position.
//==============================================
FLASHMEM void FlexIO2VGA::initGcursor(uint8_t type, uint8_t xStart,
                                      uint8_t yStart, uint8_t xEnd,
                                      uint8_t yEnd) {	
  gCursor.active = false;
  gCursor.gCursor_x  = fb_width/2; // Pixel based.
  gCursor.gCursor_y  = fb_height/2; // Pixel based.
  gCursor.x_start = xStart;
  gCursor.x_end   = xEnd;
  gCursor.y_start = yStart;
  gCursor.y_end   = yEnd;
  gCursor.type    = type;
  getGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
  moveGcursor(gCursor.gCursor_x,gCursor.gCursor_y);
}

//======================================================
// Return text cursor character based x and y positions.
//======================================================
uint16_t FlexIO2VGA::tCursorX(void) { return cursor_x+(print_window_x/font_width); }
uint16_t FlexIO2VGA::tCursorY(void) { return cursor_y+(print_window_y/font_height); }

//=========================================================
// Return graphic cursor character based x and y positions.
//=========================================================
uint16_t FlexIO2VGA::gCursorX(void) { return gCursor.gCursor_x; }
uint16_t FlexIO2VGA::gCursorY(void) { return gCursor.gCursor_y; }

//========================================
// drawPixel()
// fb is pointer to selected frame buffer.
// x and y are selected pixel coords.
// fg is foreground color.
//========================================
FLASHMEM void FlexIO2VGA::drawPixel(int16_t x, int16_t y, uint8_t fg) {
  _fb = s_frameBuffer[frameBufferIndex];

  if((x>=0) && (x<=fb_width) && (y>=0) && (y<=fb_height)) {// No neg x or y.
    unsigned int sel = (x & 1) << 2; // 4 or 0
    uint8_t c = getByte(x/2,y); // Get current 4 bit pixel pair. 
    // remove old color
    c &= (0xf0 >> sel);
    // insert new color
    c |= fg << sel;
    _fb[(y*_pitch)+(x/2)] = c;
  }
}

//========================================
// getPixel()
// fb is pointer to selected frame buffer.
// x and y are selected pixel coords.
// fg is foreground color.
//========================================
FLASHMEM uint8_t FlexIO2VGA::getPixel(uint32_t x, uint32_t y) {
  _fb = s_frameBuffer[frameBufferIndex];
  uint8_t c = getByte(x/2,y); // Get current 4 bit pixel pair. 
  // Bit == 0 == 1 then get high nibble else get low nibble.
  if(x & 1) 
    // Shift high nibble to low nibble and mask out high nibble.
    return GET_L_NIBBLE(c);
  else
    // Shift high nibble to low nibble and mask out high nibble.
    return GET_R_NIBBLE(c);
}

//=================================
// clip X to inside horizontal range.
//=================================
inline int FlexIO2VGA::clip_x(int x) {
  if(x < 0) return 0;
  if(x >= fb_width)	return fb_width - 1;
  return x;
}

//=================================
// clip Y to inside vertical range.
//=================================
inline int FlexIO2VGA::clip_y(int y) {
  if(y < 0) return 0;
  if(y >= fb_height) return fb_height - 1;
  return y;
}

//============================================
// draw a horizontal line pixel with clipping.
//============================================
FLASHMEM void FlexIO2VGA::drawHLine(int y, int x1, int x2, int color) {
  int nx1;
  int nx2;
  // line out of screen ?
  if(clip_y(y) != y) return;
  nx1 = clip_x(x1);
  nx2 = clip_x(x2);
  if(x1 <= x2) drawHLineFast(y, nx1, nx2, color);
  else drawHLineFast(y, nx2, nx1, color);
}

//==========================================
// draw a vertical line pixel with clipping.
//==========================================
FLASHMEM void FlexIO2VGA::drawVLine(int x, int y1, int y2, int color) {
  int ny1;
  int ny2;
  // line out of screen ?
  if(clip_x(x) != x) return;
  ny1 = clip_y(y1);
  ny2 = clip_y(y2);
  if(y1 <= y2) drawVLineFast(x, ny1, ny2, color);
  else drawVLineFast(x, ny2, ny1, color);
}

//=====================================================
// all drawing algorithms taken from:
// http://members.chello.at/~easyfilter/bresenham.html
//=====================================================
inline void FlexIO2VGA::delta_and_sign(int v1, int v2, int *delta, int *sign) {
  if(v2 > v1) {
    *delta = v2 - v1;
    *sign = 1;
  }	else if(v2 < v1) {
    *delta = v1 - v2;
    *sign = -1;
  }	else {
    *delta = 0;
    *sign = 0;
  }
}

//============================================
// draw a line with or without its last pixel.
//============================================
FLASHMEM void FlexIO2VGA::drawLine(int x0, int y0, int x1, int y1, int color, bool no_last_pixel) {
  int delta_x;
  int sign_x;
  int delta_y;
  int sign_y;
  int err;
  int err2;
  if(x0 == x1) {
    if(y0 == y1) drawPixel(x0, y0, color);
    else drawVLine(x0, y0, y1, color);
    return;
  }	else if(y0 == y1) {
    drawHLine(y0, x0, x1, color);
  }
  delta_and_sign(x0, x1, &delta_x, &sign_x);
  delta_and_sign(y0, y1, &delta_y, &sign_y);
  err= delta_x - delta_y;
  do {
    drawPixel(x0, y0, color);
    err2 = 2 * err;
    if(err2 > -delta_y) {
      err -= delta_y;
      x0 += sign_x;
    } else if(err2 < delta_x) {
      err += delta_x;
      y0 += sign_y;
    }
  } while( (x0 != x1) || (y0 != y1) );
  // plot last line pixel;
  if(!no_last_pixel) drawPixel(x1, y1, color);
}

//===================================================================
// draw a horizontal line pixel WITHOUT performing any clipping test.
// x1 always <= x2
//===================================================================
inline void FlexIO2VGA::drawHLineFast(int y, int x1, int x2, int color) {
  _fb = s_frameBuffer[frameBufferIndex];

  while(x1 <= x2) {
    unsigned int sel = (x1 & 1) << 2; // 4 or 0
    uint8_t c = getByte(x1/2,y); // Get current 4 bit pixel pair. 
    // remove old color
    c &= (0xf0 >> sel);
    // insert new color
    c |= color << sel;
    _fb[(y*_pitch)+(x1/2)] = c;
    x1++;
  }
}

//=================================================================
// draw a vertical line pixel WITHOUT performing any clipping test.
// y1 always <= y2
//=================================================================
inline void FlexIO2VGA::drawVLineFast(int x, int y1, int y2, int color) {
  _fb = s_frameBuffer[frameBufferIndex];

  while(y1 <= y2) {
    unsigned int sel = (x & 1) << 2; // 4 or 0
    uint8_t c = getByte(x/2,y1); // Get current 4 bit pixel pair. 
    // remove old color
    c &= (0xf0 >> sel);
    // insert new color
    c |= color << sel;
    _fb[(y1*_pitch)+(x/2)] = c;
    y1++;
  }
}

//==================
// Draw a rectangle.
//==================
FLASHMEM void FlexIO2VGA::drawRect(int x0, int y0, int x1, int y1, int color) {
  bool wasActive = false;
  if(gCursor.active) {
    gCursorOff(); // Must turn of software driven graphic cursor if on !!
    wasActive = true;
  }
  drawHLine(y0, x0, x1, color);
  drawHLine(y1, x0, x1, color);
  drawVLine(x0, y0, y1, color);
  drawVLine(x1, y0, y1, color);
  if(wasActive) gCursorOn();
}

//=========================
// Draw a rectangle filled.
//=========================
FLASHMEM void FlexIO2VGA::fillRect(int x0, int y0, int x1, int y1, int color) {
  int t;
  bool wasActive = false;
  if(gCursor.active) {
    gCursorOff(); // Must turn of software driven graphic cursor if on !!
    wasActive = true;
  }
  x0 = clip_x(x0);
  y0 = clip_y(y0);
  x1 = clip_x(x1);
  y1 = clip_y(y1);
  // increase speed if the rectangle is a single pixel, horizontal or vertical line
  if( (x0 == x1) ) {
    if(y0 == y1) {
if(wasActive) gCursorOn();
      return drawPixel(x0, y0, color);
    } else {
      if(y0 < y1) {
if(wasActive) gCursorOn();
        return drawVLineFast(x0, y0, y1, color);
      } else {
  if(wasActive) gCursorOn();
        return drawVLineFast(x0, y1, y0, color);
      }
    }
  } else if(y0 == y1) {
    if(x0 < x1) {
if(wasActive) gCursorOn();
      return drawHLineFast(y0, x0, x1, color);
    } else {
if(wasActive) gCursorOn();
      return drawHLineFast(y0, x1, x0, color);
    }
  }
  if( x0 > x1 ) {
    t = x0;
    x0 = x1;
    x1 = t;
  }
  if( y0 > y1 ) {
    t = y0;
    y0 = y1;
    y1 = t;
  }
  while(y0 <= y1) {
    drawHLineFast(y0, x0, x1, color);
    y0++;
  }
  if(wasActive) gCursorOn();
}

//===========================================================
// bitmap format must be the same as modeline.img_color_mode.
//===========================================================
void FlexIO2VGA::drawBitmap(int16_t x_pos, int16_t y_pos, uint8_t *bitmap, int16_t bitmap_width, int16_t bitmap_height) {
  int fx;
  int fy;
  int fw;
  int fh;
  int bx;
  int by;
  int off_x, off_y;
  uint8_t *bitmap_ptr;
  
  fx = clip_x(x_pos);
  // X position outside of image (right of image)
  if(fx < x_pos) return;
  // compute the number of pixels to skip at the beginning of each bitmap line and the number of pixel per line to copy
  if(fx > x_pos) {
    bx = fx - x_pos;
    fw = bitmap_width - bx;
    // X position outside of image (right of image)
    if(fw <= 0) return;
  } else {
    bx = 0;
    if( (fx + bitmap_width) >= fb_width) {
      fw = fb_width - fx;
    } else {
      fw = bitmap_width;
    }
  }
  fy = clip_y(y_pos);
  // Y position outside of image (bottom of image)
  if(fy < y_pos) return;
  // compute the number of lines to skip at the beginning of bitmap and the number of lines to copy
  if(fy > y_pos) {
    by = fy - y_pos;
    fh = bitmap_height - by;
    // Y position outside of image (right of image)
    if(fh <= 0) return;
  } else {
    by = 0;
    if( (fy + bitmap_height) >= fb_height) {
      fh = fb_height - fy;
    } else {
      fh = bitmap_height;
    }
  }

  // (fx,fy) is the destination position in the image
  // (bx,by) is the position in the bitmap
  // (fw,fh) is the size to copy
  // If a 0 value is found in the bitmap then it is substituted with the
  // currently addressed pixel in the frame buffer.
  for(off_y = 0; off_y < fh; off_y++) {
    bitmap_ptr = bitmap + (by + off_y) * bitmap_width + bx;
    for(off_x = 0; off_x < fw; off_x++) {
      if(*bitmap_ptr != 0x00) {
        // bitmap format must be the same as modeline.img_color_mode (4 bit RGBI)
        drawPixel(fx + off_x, fy + off_y, *bitmap_ptr++);
      } else{
        // bitmap format must be the same as modeline.img_color_mode (4 bit RGBI)
        drawPixel(fx + off_x, fy + off_y, getPixel(fx + off_x, fy + off_y));
        bitmap_ptr++;
      }
    }
  }
}

//==================
// Draw a circle.
//==================
FLASHMEM void FlexIO2VGA::drawCircle(float x, float y, float radius, float thickness, uint8_t color) {
  int16_t xi, yi;
  float sq_radius = radius + thickness;

  bool wasActive = false;
  if(gCursor.active) {
    gCursorOff(); // Must turn of software driven graphic cursor if on !!
    wasActive = true;
  }

  for (xi = -sq_radius; xi < sq_radius; xi++) {
    uint8_t found_state = 0;
    for (yi = -sq_radius; yi < sq_radius; yi++) {
      const float r = sqrtf(xi * xi + yi * yi);
      if(fabs(r - radius) <= thickness) {
        if(found_state == 0) found_state = 1;
        if(found_state == 2) found_state = 3;
          vga4bit.drawPixel(x + xi, y + yi, color);
      } else if(found_state == 1) {
        yi = -yi;
        found_state = 2;
      } else if(found_state == 3) {
        break;
      }
    }
  }
  if(wasActive) gCursorOn();
}

//======================
// Draw a circle filled.
//======================
FLASHMEM void FlexIO2VGA::fillCircle(float xm, float ym, float r, uint8_t color) {
  drawCircle(xm, ym, r, r, color);
/*
  for(y = -r; y <= r; y++) {
    for(x = -r; x <= r; x++) {
      if((x * x) + (y * y) <= (r * r)) {
        drawHLine(ym + y, xm + x, xm - x, color);
        if(y != 0) drawHLine(ym - y, xm + x, xm - x, color);
        break;
      }
    }
  }
*/
}

//=================
// draw a triangle
//=================
FLASHMEM void FlexIO2VGA::drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, int color) {
  bool wasActive = false;
  if(gCursor.active) {
    gCursorOff(); // Must turn of software driven graphic cursor if on !!
    wasActive = true;
  }
  drawLinex(x0, y0, x1, y1, color);
  drawLinex(x1, y1, x2, y2, color);
  drawLinex(x2, y2, x0, y0, color);
  if(wasActive) gCursorOn();
}

//=====================================================
// Fill a triangle - Bresenham method
// Original from http://www.sunshine2k.de/coding/java/
// TriangleRasterization/TriangleRasterization.html
//=====================================================
FLASHMEM void FlexIO2VGA::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int color) {
  int t1x, t2x, y, minx, maxx, t1xp, t2xp;
  bool changed1 = false;
  bool changed2 = false;
  int signx1, signx2, dx1, dy1, dx2, dy2;
  int e1, e2;

  bool wasActive = false;
  if(gCursor.active) {
    gCursorOff(); // Must turn of software driven graphic cursor if on !!
    wasActive = true;
  }

  // Sort vertices
  if (y1 > y2) {
    SWAP(y1, y2);
    SWAP(x1, x2);
  }
  if (y1 > y3) {
    SWAP(y1, y3);
    SWAP(x1, x3);
  }
  if (y2 > y3) {
    SWAP(y2, y3);
    SWAP(x2, x3);
  }
  t1x = t2x = x1;
  y = y1; // Starting points
  dx1 = x2 - x1;
  //delta_and_sign(x0, x1, &delta_x, &sign_x);
  if(dx1 < 0) {
    dx1 = -dx1;
    signx1 = -1;
  } else {
    signx1 = 1;
  }
  dy1 = y2 - y1;
  dx2 = x3 - x1;
  if(dx2 < 0) {
    dx2 = -dx2;
    signx2 = -1;
  } else {
    signx2 = 1;
  }
  dy2 = y3 - y1;
  if (dy1 > dx1) { // swap values
    SWAP(dx1, dy1);
    changed1 = true;
  }
  if(dy2 > dx2) { // swap values
    SWAP(dy2, dx2);
    changed2 = true;
  }
  e2 = dx2 >> 1;
  // Flat top, just process the second half
  if(y1 == y2) goto next;
  e1 = dx1 >> 1;
  for (int i = 0; i < dx1;) {
    t1xp = 0;
    t2xp = 0;
    if(t1x < t2x) {
      minx = t1x;
      maxx = t2x;
    } else {
      minx = t2x;
      maxx = t1x;
    }
    // process first line until y value is about to change
    while(i < dx1) {
      i++;
      e1 += dy1;
      while (e1 >= dx1) {
        e1 -= dx1;
        if(changed1)
          t1xp = signx1; //t1x += signx1;
        else
          goto next1;
      }
      if(changed1) break;
      else t1x += signx1;
    }
    // Move line
next1:
    // process second line until y value is about to change
    while(1) {
      e2 += dy2;
      while (e2 >= dx2) {
        e2 -= dx2;
        if (changed2) t2xp = signx2; //t2x += signx2;
        else goto next2;
      }
      if(changed2) break;
        else t2x += signx2;
    }
next2:
    if(minx > t1x) minx = t1x;
    if(minx > t2x) minx = t2x;
    if(maxx < t1x) maxx = t1x;
    if(maxx < t2x) maxx = t2x;
    drawHLine(y, minx, maxx, color);
    // Now increase y
    if(!changed1) t1x += signx1;
    t1x += t1xp;
    if(!changed2) t2x += signx2;
    t2x += t2xp;
    y += 1;
    if(y == y2) break;
  }
next:
  // Second half
  dx1 = x3 - x2;
  if(dx1 < 0) {
    dx1 = -dx1;
    signx1 = -1;
  } else {
    signx1 = 1;
  }
  dy1 = y3 - y2;
  t1x = x2;
  if(dy1 > dx1) {    // swap values
    SWAP(dy1, dx1);
    changed1 = true;
  }	else {
    changed1 = false;
  }
  e1 = dx1 >> 1;
  for (int i = 0; i <= dx1; i++) {
    t1xp = 0;
    t2xp = 0;
    if(t1x < t2x) {
      minx = t1x;
      maxx = t2x;
    } else {
      minx = t2x;
      maxx = t1x;
    }
    // process first line until y value is about to change
    while(i < dx1) {
      e1 += dy1;
      while (e1 >= dx1)	{
        e1 -= dx1;
        if(changed1) {
          t1xp = signx1;   //t1x += signx1;
          break;
        } else
          goto next3;
      }
      if(changed1) break;
      else t1x += signx1;
      if(i < dx1) i++;
    }
next3:
    // process second line until y value is about to change
    while (t2x != x3) {
      e2 += dy2;
      while (e2 >= dx2) {
        e2 -= dx2;
        if(changed2) t2xp = signx2;
        else goto next4;
      }
      if(changed2) break;
      else t2x += signx2;
    }
next4:
    if(minx > t1x) minx = t1x;
    if(minx > t2x) minx = t2x;
    if(maxx < t1x) maxx = t1x;
    if(maxx < t2x) maxx = t2x;
    drawHLine(y, minx, maxx, color);
    // Now increase y
    if(!changed1) t1x += signx1;
    t1x += t1xp;
    if(!changed2) t2x += signx2;
    t2x += t2xp;
    y += 1;
    if(y > y3) {
      if(wasActive) gCursorOn();
      return;
    }
  }
  if(wasActive) gCursorOn();
}

//==============================================================
// Draw ellipse.
// x0, y0 = center of the ellipse
// x1, y1 = upper right edge of the bounding box of the ellipse
// ************* Fails with certain params ********************
//==============================================================
FLASHMEM void FlexIO2VGA::drawEllipse(int _x0, int _y0, int _x1, int _y1, int color) {
  //----------------------------------------
  // Added the following:
  //----------------------------------------
  int tx, ty;
  tx = _x1;
  ty = _y1;
  _x1 += _x0;
  _y1 += _y0;
  _x0 -= tx;
  _y0 -= ty;
  //----------------------------------------

  int halfHeight = abs(_y0 - _y1) / 2;
  int halfWidth = abs(_x0 - _x1) / 2;
  int x0 = (_x0 < _x1 ? _x0 : _x1);
  int y0 = (_y0 < _y1 ? _y0 : _y1) + 2 * halfHeight;
  int x1 = x0 + halfWidth * 2;
  int y1 = y0 - halfHeight * 2;
  int a = abs(x1-x0);
  int b = abs(y1-y0);
  int b1 = b & 1; // values of diameter
  long dx = 4 * (1 - a) * b * b;
  long dy = 4 * (b1 + 1) * a * a; // error increment
  long err = dx + dy + b1 * a * a;
  long e2; // error of 1.step

  bool wasActive = false;
  if(gCursor.active) {
    gCursorOff(); // Must turn of software driven graphic cursor if on !!
    wasActive = true;
  }
  // if x1,y1 is not the correct edge, try... something
  if(x0 > x1) {
    x0 = x1;
    x1 += a;
  }
  if(y0 > y1) {
    y0 = y1;
  }
  y0 += (b + 1) / 2;
  y1 = y0 - b1;	// starting pixel
  a *= 8*a;
  b1 = 8*b*b;
  do {
    drawPixel(x1, y0, color); //	I. Quadrant
    drawPixel(x0, y0, color); //  II. Quadrant
    drawPixel(x0, y1, color); // III. Quadrant
    drawPixel(x1, y1, color); //  IV. Quadrant
    e2 = 2*err;
    if(e2 <= dy) {
      y0++;
      y1--;
      dy += a;
      err += dy;
    }  // y step 
    if((e2 >= dx) || (2*err > dy)) {
      x0++;
      x1--;
      dx += b1;
      err += dx;
    } // x step
  } while (x0 <= x1);
	
  while ((y0 - y1) < b) {  // too early stop of flat ellipses a=1
    drawPixel(x0 - 1, y0, color); // -> finish tip of ellipse
    drawPixel(x1 + 1, y0++, color); 
    drawPixel(x0 - 1, y1, color);
    drawPixel(x1 + 1, y1--, color); 
  }
  if(wasActive) gCursorOn();
}

//==============================================================
// Draw ellipse filled.
// x0, y0 = center of the ellipse.
// x1, y1 = upper right edge of the bounding box of the ellipse.
// ************* Fails with certain params ********************
// Example: vga4bit.fillEllipse(516,419,59,61,VGA_BRIGHT_WHITE);
//          vga_timing = t640x480x60. FAILS...
//==============================================================
FLASHMEM void FlexIO2VGA::fillEllipse(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
  int x;
  int y;

  //----------------------------------------
  // Added the following:
  //----------------------------------------
  int tx, ty;
  tx = x1;
  ty = y1;
  x1 += x0;
  y1 += y0;
  x0 -= tx;
  y0 -= ty;
  //----------------------------------------

  int half_height = abs(y0 - y1) / 2;
  int half_width = abs(x0 - x1) / 2;
  int center_x = (x0 < x1 ? x0 : x1) + half_width;
  int center_y = (y0 < y1 ? y0 : y1) + half_height;

//x*height*x*height+y*width*y*width <= width*height*width*height
  bool wasActive = false;
  if(gCursor.active) {
    gCursorOff(); // Must turn of software driven graphic cursor if on !!
    wasActive = true;
  }

  for(y = -half_height; y <= 0; y++) {
    for(x = -half_width; x <= 0; x++) {
      if( (x * x * half_height * half_height + y * y * half_width * half_width) <=
          (half_height * half_height * half_width * half_width)) {
        drawHLine(center_y + y, center_x + x, center_x - x, color);
        if(y != 0) drawHLine(center_y - y, center_x + x, center_x - x, color);
          break;
      }
    }
  }
  if(wasActive) gCursorOn();
}


// ------------------------------------------------------
// copy area s_x,s_y of w*h pixels to destination d_x,d_y
// ------------------------------------------------------
FLASHMEM void FlexIO2VGA::copy(int s_x, int s_y, int d_x, int d_y, int w, int h) {
  int c_s_x;
  int c_s_y;
  int c_d_x;
  int c_d_y;
  int c_w;
  int c_h;
  int error;
  int sxpos;
  int sypos;
  int dxpos;
  int dypos;
  int dx;
  int dy;
  uint8_t c = 0;  
  int off_x;
  int off_y;
  
  // nothing to copy ?
  if((w <= 0) || (h <= 0)) return;
  
  c_s_x = s_x;
  c_s_y = s_y;
  c_d_x = d_x;
  c_d_y = d_y;
  c_w = w;
  c_h = h;

  // let's do some clipping to avoid overflow
  // 1) adjust position and size of source area according to screen size
  if(c_s_x < 0) {
    c_w += c_s_x;
    c_d_x -= c_s_x;
    c_s_x = 0;
  }
  if(c_s_y < 0) {
    c_h += c_s_y;
    c_d_x -= c_s_y;
    c_s_y = 0;
  }
  error = c_s_x + c_w;
  if(error >= fb_width) c_w = fb_width - c_s_x;
  error = c_s_y + c_h;
  if(error >= fb_height) c_h = fb_height - c_s_y;
  // 2) adjust destination position and source size according to screen size
  if(c_d_x < 0) {
    c_w += c_d_x;
    c_s_x -= c_d_x;
    c_d_x = 0;
  }
  if(c_d_y < 0) {
    c_h += c_d_y;
    c_s_y -= c_d_y;
    c_d_y = 0;
  }
  error = c_d_x + c_w;
  if(error >= fb_width) c_w = fb_width - c_d_x;
  error = c_d_y + c_h;
  if(error >= fb_height) c_h = fb_height - c_d_y;
  // nothing left to copy ?
  if((c_w <= 0) || (c_h <= 0)) return;
  if(c_d_y > c_s_y) {
    // copy from last line
    sypos = c_s_y + c_h - 1;
    dypos = c_d_y + c_h - 1;
    dy = -1;
  } else {
    // copy from first line
    sypos = c_s_y;
    dypos = c_d_y;
    dy = 1;
  }
  if(c_d_x > c_s_x) {
      // copy from last line pixel
      sxpos = c_s_x + c_w - 1;
      dxpos = c_d_x + c_w - 1;
      dx = -1;
  } else {
    // copy from first line pixel
    sxpos = c_s_x;
    dxpos = c_d_x;
    dx = 1;
  }
  for(off_y = 0; off_y < c_h; off_y++) {
    for(off_x = 0; off_x < c_w; off_x++) {
      // Get existing gpixel info (byte).
      c = getByte((sxpos + off_x * dx)/2, sypos + off_y * dy);
      // Bit == 0 == 1 then get high nibble else get low nibble.
      if(off_x & 1) 
      // Shift high nibble to low nibble and mask out high nibble.
        drawPixel((dxpos + off_x * dx), dypos + off_y * dy,GET_L_NIBBLE(c));
      else
      // Shift high nibble to low nibble and mask out high nibble.
        drawPixel((dxpos + off_x * dx), dypos + off_y * dy,GET_R_NIBBLE(c));
    }
  }
}

//================
// Unused...
//================
FLASHMEM void FlexIO2VGA::init() {

}

//================
// Text methods
//================
FLASHMEM void FlexIO2VGA::init_text_settings() {
  _fb = s_frameBuffer[frameBufferIndex];
   
  set_next_buffer(s_frameBuffer[frameBufferIndex], _pitch, false);
  bpp = 4;
  cursor_x = 0;
  cursor_y = 0;

  font_width = 8;	// font width != 8 is not supported
  font_height = 16;
  promp_size = 0;  
  print_window_x = 0;
  print_window_y = 0;
  print_window_w = fb_width / font_width;
  print_window_h = fb_height / font_height;

  foreground_color = VGA_BRIGHT_WHITE;
  background_color = VGA_BLUE;
  transparent_background = false;
  clear(background_color);

  tCursor.tCursor_x = 0;
  tCursor.tCursor_y = 0;
  tCursor.active = false;
  vga4bit.initCursor(1,0,8,8,true,30); 
  vga4bit.initGcursor(1,1,0,8,8);
}

//====================================================
// Get a character from frame buffer:
// x = starting x position in frame buffer.
// y = starting y position in frame buffer.
// font_width / 2 (2 pixels per byte).
// font_height * 2 (compensate for font_width / 2).
//====================================================
FLASHMEM void FlexIO2VGA::getChar(int16_t x, int16_t y, uint8_t *buf) {
  for(int16_t i = 0; i < font_height*2; i++) {
    for(int16_t j = 0; j < (font_width/2); j++) {
      *buf++ = getByte((x * font_width/2)+j, (y * font_height)+i);
    }
  }
}

//====================================================
// Write a character to frame buffer:
// x = starting x position in frame buffer.
// y = starting y position in frame buffer.
// font_width / 2 (2 pixels per byte).
// font_height * 2 (compensate for font_width / 2).
//====================================================
FLASHMEM void FlexIO2VGA::putChar(int16_t x, int16_t y, uint8_t *buf) {
  for(int16_t i = 0; i < font_height*2; i++) {
    for(int16_t j = 0; j < (font_width/2); j++) {
      putByte((x * font_width/2)+j, (y * font_height)+i, *buf++);
    }
  }
}

//====================================================
// Get a character from frame buffer:
// x = starting x position in frame buffer.
// y = starting y position in frame buffer.
// font_width / 2 (2 pixels per byte).
// font_height * 2 (compensate for font_width / 2).
//====================================================
FLASHMEM void FlexIO2VGA::getGptr(int16_t x, int16_t y, uint8_t *buf) {
//  for(int16_t i = 0; i < font_height*2+1; i++) {
  for(int16_t i = 0; i < font_height; i++) {
    for(int16_t j = 0; j < (font_width/2)+1; j++) {
      *buf++ = getByte((x/2)+j, y+i);
    }
  }
}

//====================================================
// Write a character to frame buffer:
// x = starting x position in frame buffer.
// y = starting y position in frame buffer.
// font_width / 2 (2 pixels per byte).
// font_height * 2 (compensate for font_width / 2).
//====================================================
FLASHMEM void FlexIO2VGA::putGptr(int16_t x, int16_t y, uint8_t *buf) {
//  for(int16_t i = 0; i < font_height*2+1; i++) {
  for(int16_t i = 0; i < font_height; i++) {
    for(int16_t j = 0; j < (font_width/2)+1; j++) {
      putByte((x/2)+j, y+i, *buf++);
    }
  }
}

//==========================================
// getByte()
// Each pixel is 4bits. One byte is returned
// which represents two pixels. High and low
// nibbles.
//==========================================
FLASHMEM uint8_t FlexIO2VGA::getByte(uint32_t x, uint32_t y) {

  _fb = s_frameBuffer[frameBufferIndex];
  return _fb[(y*_pitch)+x];
}

//==========================================
// putByte()
// Each pixel is 4bits. One byte is written
// which represents two pixels. High and low
// nibbles.
//==========================================
FLASHMEM  void FlexIO2VGA::putByte(uint32_t x, uint32_t y, uint8_t byte) {
  _fb = s_frameBuffer[frameBufferIndex];
  _fb[(y*_pitch)+x] = byte;
}

//=======================================
//  Write a count bytes to linear memory.
//=======================================
FLASHMEM void FlexIO2VGA::writeVmem(uint8_t *buf, uint32_t vMem, uint32_t count) {
  _fb = s_frameBuffer[frameBufferIndex] + vMem;
  while(count) {
    *_fb++ = *buf++;
    count--;
  }	
}
	  
//========================================
//  Read a count bytes from linear memory.
//========================================
FLASHMEM void FlexIO2VGA::readVmem(uint32_t vMem, uint8_t *buf, int32_t count) {
  _fb = s_frameBuffer[frameBufferIndex] + vMem;
  while(count) {
    *buf++ = *_fb++;
    count--;
  }
}

//====================
// Set prompt size.
// Default prompt ">".
//====================
FLASHMEM void FlexIO2VGA::setPromptSize(uint16_t ps) {
//  promp_size = (ps * font_width) + print_window_w;	
  promp_size = (ps * font_width);	
}

//===========================================
// Set font size. Just selecting font height.
// Valid sizes: 8 or 16 for now.
//===========================================
FLASHMEM int FlexIO2VGA::setFontSize(uint8_t fsize, bool runflag) {
  if((fsize != 8) && (fsize != 16)) return -1;
  font_height = fsize;	
  promp_size = 0;
  if(!runflag) clear(background_color);
  return (int)fsize;
}

//===========================================
// Draw a string. Default to right direction.
//===========================================
FLASHMEM void FlexIO2VGA::drawText(int16_t x, int16_t y, const char * text, uint8_t fgcolor, uint8_t bgcolor) {
  drawText(x, y, text, fgcolor, bgcolor, VGA_DIR_RIGHT);
}

//=========================================
// Draw a string in one of four directions.
// Right, Left, Up and Down.
//=========================================
FLASHMEM void FlexIO2VGA::drawText(int16_t x, int16_t y, const char * text, uint8_t fgcolor, uint8_t bgcolor, vga_text_direction dir) {
  uint8_t t;
  int i,j;
  const uint8_t *charPointer;
  uint8_t b;
  uint8_t pix;
  
  while ((t = *text++)) {
    if(font_height == 8)
      charPointer = &font_8x8[t*font_height];
    else
      charPointer = &font_8x16[t*font_height];
    for(j = 0; j < font_height; j++) {
      b = *charPointer++;
      for(i = 0; i < font_width; i++) {
       pix = b & (128 >> i);
        // pixel to draw or non transparent background color ?
        if((pix) || (bgcolor != -1)) {
          switch(dir) {
            case VGA_DIR_RIGHT:
              drawPixel(x + i, y + j, (pix ? fgcolor : bgcolor));
              break;
            case VGA_DIR_TOP:
              drawPixel(x + j, y - i, (pix ? fgcolor : bgcolor));
              break;
            case VGA_DIR_LEFT:
              drawPixel(x - i, y - j, (pix ? fgcolor : bgcolor));
              break;
            case VGA_DIR_BOTTOM:
              drawPixel(x - j, y + i, (pix ? fgcolor : bgcolor));
              break;
          }
	    }
      }
    }
    switch(dir) {
      case VGA_DIR_RIGHT:
        x += font_width;
        break;
      case VGA_DIR_TOP:
        y -= font_width;
        break;
      case VGA_DIR_LEFT:
        x -= font_width;
        break;
      case VGA_DIR_BOTTOM:
        y += font_width;
        break;
    }
  }
}

//=========================================
// Define text printing window.
// x, y, width and height are in pixels.
//=========================================
FLASHMEM void FlexIO2VGA::setPrintWindow(int x, int y, int width, int height) {
  if(x < 0)
    print_window_x = 0;
  else if(x >= (fb_width - font_width))
    print_window_x = fb_width - font_width - 1;
  else
    print_window_x = x;
  if(y < 0)
    print_window_y = 0;
  else if(y >= (fb_height - font_height))
    print_window_y = fb_height - font_height - 1;
  else
    print_window_y = y;
  if(width < font_width) width = font_width;
  if(height < font_height) height = font_height;
  print_window_w = (width / font_width) / (double_width ? 2:1);
  print_window_h = height / font_height / (double_height ? 2:1);
}

//==================================================
// Clear a text window to current backgraound color.
//==================================================
FLASHMEM void FlexIO2VGA::clearPrintWindow() {
  fillRect(print_window_x, print_window_y, print_window_x +
          (print_window_w+1) * font_width, print_window_y +
          (print_window_h+1) * font_height, background_color);
  cursor_x = 0;
  cursor_y = 0;
  getChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
  getGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
}

//=====================
// Unset a text window.
//=====================
FLASHMEM void FlexIO2VGA::unsetPrintWindow() {
  print_window_x = 0;
  print_window_y = 0;
  print_window_w = fb_width / font_width;
  print_window_h = fb_height / font_height;
}

//=========================================
// Scroll up text one line. (Slow!)
//=========================================
FLASHMEM void FlexIO2VGA::scrollPrintWindow() {
  // move the 2nd line and the following ones one line up
  Vscroll(print_window_x, print_window_y + font_height, 
          print_window_w * font_width, (print_window_h - 1) *
          font_height, -font_height, background_color);
}

//=========================================
// Scroll up text one line. (Slow!)
//=========================================
FLASHMEM void FlexIO2VGA::scrollUp() {
  // move the 2nd line and the following ones one line up
  Vscroll(print_window_x, print_window_y + font_height, 
          print_window_w * font_width, (print_window_h - 1) *
          font_height, -font_height, background_color);
}

//=========================================
// Scroll down text one line. (Slow!)
//=========================================
FLASHMEM void FlexIO2VGA::scrollDown() {
  // move the 2nd line and the following ones one line up
  Vscroll(print_window_x, print_window_y + font_height, 
          print_window_w * font_width, (print_window_h - 1) *
          font_height, font_height, background_color);
}

//============================================
// Update text cursor x,y character positions.
//============================================
FLASHMEM void FlexIO2VGA::updateTCursor(int column, int line) {
  tCursor.tCursor_x = column*font_width+print_window_x;
  tCursor.tCursor_y = line*font_height+print_window_y;	
  getChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
}

//================================================
// Draw text cursor at current character position.
// Cursor size vertically and horizontally are
// based on font sizes 8x8 or 8x16. (8x16 max)
//================================================
FLASHMEM void FlexIO2VGA::drawCursor(int color) {
  fillRect(tCursor.tCursor_x+tCursor.x_start, tCursor.tCursor_y+tCursor.y_start,
           tCursor.tCursor_x+tCursor.x_end-1, tCursor.tCursor_y+tCursor.y_end-1,
           color);
}

//=========================================
// Move character position to column/line.
//=========================================
FLASHMEM void FlexIO2VGA::textxy(int column, int line) {
  bool isActive = false;
  if(tCursor.active) isActive = true;
  getChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
  cursorOff();
  if(column < 0)
    cursor_x = 0;
  else if(column >= print_window_w)
    cursor_x = print_window_w - 1;
  else
    cursor_x = column;
  if(line < 0)
    cursor_y = 0;
  else if(line >= print_window_h)
    cursor_y = print_window_h - 1;
  else
   cursor_y = line;
  updateTCursor(cursor_x, cursor_y);
  if(isActive) cursorOn();
}

//================================================
// Draw graphic cursor at current character position.
// Cursor size vertically and horizontally are
// based on font sizes 8x8 or 8x16. (8x16 max)
//================================================
FLASHMEM void FlexIO2VGA::drawGcursor(int color) {
  if(gCursor.active) {
    getGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
    if(gCursor.type == BLOCK_CURSOR) {
      fillRect(gCursor.gCursor_x+gCursor.x_start, gCursor.gCursor_y+gCursor.y_start,
               gCursor.gCursor_x+gCursor.x_end-1, gCursor.gCursor_y+gCursor.y_end-1,
               color);
    } else {
      drawBitmap(gCursor.gCursor_x, gCursor.gCursor_y, (uint8_t *)arrow[gCursor.type], 8, 16);
    }
  }
}

//=============================================
// Move graphic cursor position to column/line.
//=============================================
FLASHMEM void FlexIO2VGA::moveGcursor(int16_t column, int16_t line) {
  if(gCursor.active) {
    putGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
    gCursor.gCursor_x = column;
    gCursor.gCursor_y = line;
    drawGcursor(foreground_color);
  }
}

//======================================================================
// Scroll an area of the screen, top left corner (x,y),
// width w, height h by (dx,dy) pixels. If dx>0 scrolling is right, dx<0
// is left. dy>0 is down, dy<0 is up.
// Empty area is filled with color col.
// (only when horizontal (dy=0) or vertical scroll (dx=0))
//======================================================================
FLASHMEM void FlexIO2VGA::scroll(int x, int y, int w, int h, int dx, int dy,int col) {
	if(dy == 0)
	{
		if(dx == 0)
			return;

		Hscroll(x, y, w, h, dx, col);
	}
	else if(dx == 0)
	{
		Vscroll(x, y, w, h, dy, col);
	}
	else
	{
		copy(x, y, x + dx, y + dy, w, h);
	}

}

//======================================================================
// Hscroll used by scroll(). Scrolls right or left.
//======================================================================
inline void FlexIO2VGA::Hscroll(int x, int y, int w, int h, int dx, int col)
{
  copy(x, y, x + dx, y, w, h);
  // fill empty area created with col
  if(dx > 0) {
    // move to the right => fill area on the left side of source area
    fillRect(x, y, x + dx - 1 , y + h, col);
  }	else {
    // move to the left => fill area on the right side of source area
    fillRect(x + w + dx, y, x + w - 1 , y + h, col);
  }
}

//======================================================================
// Vscroll used by scroll(). Scrolls up or down.
//======================================================================
inline void FlexIO2VGA::Vscroll(int x, int y, int w, int h, int dy, int col) {
  copy(x, y, x, y + dy, w, h);
  // fill empty area created with col
  if(dy > 0) {
    // move to the bottom => fill area on the top side of source area
    fillRect(x, y, x + w - 1 , y + dy, col);
  }	else {
    // move to the top => fill area on the bottom side of source are
    fillRect(x, y + h + dy, x + w - 1 , y + h, col);
  }
}

//======================================================================
// Set forground and background to one of 16 colors. See vga_4bit_T4.h
// for color defs.
//======================================================================
FLASHMEM void FlexIO2VGA::textColor(uint8_t fgc, uint8_t bgc) {
     setForegroundColor(fgc);	
     setBackgroundColor(bgc);	
}

//======================================================================
// setForgroundColor to one of 16 colors. See vga_4bit_T4.h for color defs.
//======================================================================
FLASHMEM void FlexIO2VGA::setForegroundColor(int8_t fg_color) { // RGBI format
  foreground_color = fg_color;
}

//======================================================================
// setBackgroundColor to one of 16 colors. See vga_4bit_T4.h for color defs.
// if bg_color == -1 then set transparent_background = true. (Untested)
//======================================================================
FLASHMEM void FlexIO2VGA::setBackgroundColor(int8_t bg_color) { // RGBI format
  if(bg_color == -1) {
    transparent_background = true;
  } else {
    transparent_background = false;
    background_color = bg_color;
  }
}

//===============================================
// Write a single caracter to the display buffer.
// Proccess control characters. Ignore '\r' char.
//===============================================
FLASHMEM size_t FlexIO2VGA::write(uint8_t c) {
  char buf[2];
  bool isActive = false;

  // If cursor is active, set flag and turn off cursor.
  if(tCursor.active) {
    cursorOff();
    isActive = true;
  }
  switch(c) {
    case '\r':
//      cursor_x = 0;  ignore carriage return.
      break;
    case '\n': // Proccess linefeed.
      cursor_x = 0;
      cursor_y ++;
      if(cursor_y >= print_window_h) {
        scrollPrintWindow(); // Scroll up one line.
        cursor_y = print_window_h - 1;
      }
      break;
    case 127: // Destructive Backspace
      if((cursor_x == 0) && (cursor_y == 0)) break;
      if(cursor_x > 0) {
        cursor_x--;
        updateTCursor(cursor_x, cursor_y);
	    write(0x20);
      }
      cursor_x--;
      if((cursor_y > 0) && (cursor_x < 0)) {
		cursor_y--;
		cursor_x += print_window_w+1;
	  }
	  break;
    case '\t': // Do a tab.
      write(' ');
      // prevent neverending loop if print window width is too small to contain a TAB
      if(print_window_w >= TABSIZE) {
        while(cursor_x & (TABSIZE-1)) write(' ');
      }
      break;
    case 12: // Form feed.
      clearPrintWindow();
    break;
    default:
      // not enough space on the line ?
      if(cursor_x >= print_window_w) write('\n'); // Do linefeed.
      buf[0] = c;
      buf[1] = '\0';
      drawText(print_window_x + cursor_x * font_width,
               print_window_y + cursor_y * font_height, buf, 
               foreground_color, background_color, VGA_DIR_RIGHT);
      cursor_x++;
  }
  updateTCursor(cursor_x, cursor_y);
  if(isActive) cursorOn();
  return 1;
}

//==========================================
// Write a size string to display buffer.
//==========================================
FLASHMEM size_t FlexIO2VGA::write(const char *buffer, size_t size) {
  size_t i;
  for(i = 0; i < size; i++) write(buffer[i]);
  return size;
}

//==========================================
// Wrtie a string to the status line.
//==========================================
FLASHMEM void FlexIO2VGA::slWrite(int16_t x,  uint16_t fgcolor,
                      uint16_t bgcolor, const char * text) {
  drawText(x*font_width,fb_height-font_height , text, fgcolor, bgcolor, VGA_DIR_RIGHT);
}

//==========+================================
// Clear the status line. The 800x600 mode
// needs fb_height adjusted to 600 - 8 as
// 600 / (font_height == 16) = 37.5 character
// lines. We subtract 8 from fb_height to get
// an even 37 character lines. A font_height
// of 8 gives an even 75 character lines.
//=========+=================================
FLASHMEM void FlexIO2VGA::clearStatusLine(uint8_t bgc) {
  if((fb_height == 600) && (font_height == 16)) {
    fillRect(0, fb_height-font_height-8, fb_width, fb_height-8,bgc);
  } else {
	fillRect(0, fb_height-font_height, fb_width, fb_height,bgc);	
  }
}

//==========================================
// Write frame buffer to VGA memory. (DMA)
//==========================================
void FlexIO2VGA::fbUpdate(bool wait) {
  arm_dcache_flush_delete(s_frameBuffer[frameBufferIndex], fb_height*_pitch);
  set_next_buffer(s_frameBuffer[frameBufferIndex], _pitch, wait);
}

//==================================================
// Support function for VT100: Clear to End Of Line.
//==================================================
void FlexIO2VGA::clreol(void) {
  int16_t tempX = cursor_x;
  int16_t tempY = cursor_y;
  bool isActive = false;
  if(tCursor.active) isActive = true;

  for(int i = 0; i < (print_window_w-tempX); i++) {
	write(0x20);
    cursorOff();
  }
  textxy(tempX,tempY);
  if(isActive) cursorOn();

}

//====================================================
// Support function for VT100: Clear to End Of Screen.
//====================================================
void FlexIO2VGA::clreos(void)
{
  int16_t tempX = cursor_x;
  int16_t tempY = cursor_y;

  bool isActive = false;
  if(tCursor.active) isActive = true;

  clreol();

  for(uint16_t y = 0; y < (print_window_h - tempY); y++)
    for(int16_t x = 0; x < (print_window_w-tempX); x++)
      write(0x20); 

  textxy(tempX,tempY);
  if(isActive) cursorOn();
}

//========================================================
// Support function for VT100: Clear to beginning of line.
//========================================================
void FlexIO2VGA::clrbol(void)
{
  int16_t tempX = cursor_x;
  int16_t tempY = cursor_y;

  bool isActive = false;
  if(tCursor.active) isActive = true;

  for(int16_t x = tempX; x > 0; x--) write(0x7f);

  textxy(tempX,tempY);
  if(isActive) cursorOn();
}

//=========================================================
// Support function for VT100: Clear to begining of Screen.
//=========================================================
void FlexIO2VGA::clrbos(void)
{
  int16_t tempX = cursor_x;
  int16_t tempY = cursor_y;
  
  bool isActive = false;
  if(tCursor.active) isActive = true;

  clrbol();
  textxy(0,0);
  
  for(uint16_t y = 0; y < tempY; y++)
    for(int16_t x = 0; x < print_window_w; x++)
      write(0x20); 

  textxy(tempX,tempY);
  if(isActive) cursorOn();
}

//========================================
// Support function for VT100: Clear Line.
//========================================
void FlexIO2VGA::clrlin(void)
{
  int16_t tempX = cursor_x;
  int16_t tempY = cursor_y;
  bool isActive = false;
  if(tCursor.active) isActive = true;

  textxy(0,tempY);
  for(int16_t x = 0; x < print_window_w; x++) write(0x20);

  textxy(tempX,tempY);
  if(isActive) cursorOn();
}

extern FlexIO2VGA vga4bit;

FASTRUN void FlexIO2VGA::ISR(void) {
  uint32_t timStatus = FLEXIO2_TIMSTAT & 0xFF;
  FLEXIO2_TIMSTAT = timStatus;

  if (timStatus & (1<<5)) {
    vga4bit.TimerInterrupt();
  }

  asm volatile("dsb");
}

/* END VGA driver code */
