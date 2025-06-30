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

// Frame buffer configurations. Un-comment one at a time.
DMAMEM uint8_t frameBuffer0[(MAX_HEIGHT+1)*(MAX_WIDTH+STRIDE_PADDING)];
//uint8_t frameBuffer0[(MAX_HEIGHT+1)*(MAX_WIDTH+STRIDE_PADDING)];
//EXTMEM uint8_t frameBuffer0[(MAX_HEIGHT+1)*(MAX_WIDTH+STRIDE_PADDING)];
//static uint8_t frameBuffer1[(MAX_HEIGHT+1)*(MAX_WIDTH+STRIDE_PADDING)];

static uint8_t* const s_frameBuffer[1] = {frameBuffer0};

static uint32_t frameBufferIndex = 0;
static int fb_width;
static int fb_height;
static size_t _pitch;  
uint8_t currentFont[256*16] DMAMEM;
text_cursor tCursor;
graphic_cursor gCursor;

PolyDef_t PolySet;  // will contain a polygon data

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
FLASHMEM void FlexIO2VGA::begin(const vga_timing& mode, bool half_width, bool half_height, unsigned int bpp) {
  frameCount = 0;
  IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_02 = 4; // FLEXIO2_D2    RED
  IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_01 = 4; // FLEXIO2_D1    GREEN
  IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_00 = 4; // FLEXIO2_D0    BLUE
  IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_03 = 4; // FLEXIO2_D3    INTENSITY
  IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_13 = 4; // FLEXIO2_D29   VSYNC
  IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_12 = 4; // FLEXIO2_D28   HSYNC

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
  if(!initialized) {
	init_text_settings();
	initialized = true;
  }
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
          drawTcursor(tCursor.color);
        } else {
          putChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
        }
      }
    } else {

      drawTcursor(tCursor.color);
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

//==============================================
// Select and set screen mode.
//==============================================
FLASHMEM void FlexIO2VGA::setScreenMode(const vga_timing& mode, bool half_height,
                                         bool half_width, unsigned int bpp) {
  stop();
  tCursorOff();
  begin(mode,half_height, half_width, bpp);
  print_window_x = 0;
  print_window_y = 0;
  print_window_w = fb_width / font_width;
  print_window_h = fb_height / font_height;
  clear(background_color);
  textxy(0,0);
  tCursorOn(); 
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
// Get frame buffer text width.
//------------------------
FLASHMEM uint16_t  FlexIO2VGA::getTwidth(void) { return fb_width/font_width; }

//-------------------------
// Get frame buffer text height.
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
FLASHMEM void FlexIO2VGA::tCursorOn(void) {
  tCursor.active = true;	
  getChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
}

//===============================================
// Turn cursor off and kill any displayed cursor.
//===============================================
FLASHMEM void FlexIO2VGA::tCursorOff(void) {
  tCursor.active = false;	
  putChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
}

//===================================
// Turn gCursor on and display cursor.
//===================================
FLASHMEM void FlexIO2VGA::gCursorOn(void) {
  gCursor.active = true;	
  getGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
  drawGcursor(gCursor.color);
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
  tCursor.color   = foreground_color;
  getChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
}


//=====================================
// Set text cursor color.
//=====================================
void FlexIO2VGA::setTcursorColor(uint8_t cursorColor) {
  bool isActive = false;
  if(tCursor.active) isActive = true;
  tCursorOff();
  tCursor.color = cursorColor;
  putChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
  if(isActive) {
    drawTcursor(tCursor.color);
    tCursorOn();
  }
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
  putGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
  drawGcursor(gCursor.color);
}

//=====================================
// Set graphic cursor color.
//=====================================
void FlexIO2VGA::setGcursorColor(uint8_t cursorColor) {
  gCursor.color = cursorColor;
  putGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
  drawGcursor(gCursor.color);
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
  gCursor.color   = foreground_color;
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

//=============================================
// Set block cursor dimensions.
// xStart = cursor starting x position.
// yStart = cursor starting y position.
// xEnd   = cursor ending x position.
// yEnd   = cursor ending y position.
// type   0 = text, 1 = graphic else -1 error.
//==============================================
FLASHMEM int8_t FlexIO2VGA::setBlkCursorDims(uint8_t xStart,
                                      uint8_t yStart, uint8_t xEnd,
                                      uint8_t yEnd,uint8_t type) {	
  switch(type) {
    case 0:
      tCursor.x_start = xStart;
      tCursor.x_end   = xEnd;
      tCursor.y_start = yStart;
      tCursor.y_end   = yEnd;
      break;
    case 1:
      gCursor.x_start = xStart;
      gCursor.x_end   = xEnd;
      gCursor.y_start = yStart;
      gCursor.y_end   = yEnd;
      break;
    default:
      return -1; // Type not supported.
  }
  putGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
  drawGcursor(gCursor.color);
  return 0;
}
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

//--------------------------------------------------------------
// Draw a horizontal line
// x1,y1   : starting point
// lenght  : lenght in pixels
// color   : 16bits color
//--------------------------------------------------------------
FLASHMEM void FlexIO2VGA::draw_h_line(int16_t x, int16_t y, int16_t lenght, uint8_t color){
	drawLine(x , y , x + lenght , y , color, false);
}

//--------------------------------------------------------------
// Draw a vertical line
// x1,y1   : starting point
// lenght  : lenght in pixels
// color   : 16bits color
//--------------------------------------------------------------
FLASHMEM void FlexIO2VGA::draw_v_line(int16_t x, int16_t y, int16_t lenght, uint8_t color){
	drawLine(x , y , x , y + lenght , color, false);
}

//==================
// Draw a rectangle.
//==================
FLASHMEM void FlexIO2VGA::drawRect(int x0, int y0, int x1, int y1, int color) {
  bool wasActive = false;
  if(gCursor.active) {
    gCursorOff(); // Must turn off software driven graphic cursor if on !!
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
  x0 = clip_x(x0);
  y0 = clip_y(y0);
  x1 = clip_x(x1);
  y1 = clip_y(y1);
  // increase speed if the rectangle is a single pixel, horizontal or vertical line
  if( (x0 == x1) ) {
    if(y0 == y1) {
      return drawPixel(x0, y0, color);
    } else {
      if(y0 < y1) {
        return drawVLineFast(x0, y0, y1, color);
      } else {
        return drawVLineFast(x0, y1, y0, color);
      }
    }
  } else if(y0 == y1) {
    if(x0 < x1) {
      return drawHLineFast(y0, x0, x1, color);
    } else {
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
}

//======================================================================
//  Displays a Rectangle at a given Angle.
//  centerx			: specifies the center of the Rectangle.
//	centery
//  w,h 	        : specifies the size of the Rectangle.
//	angle			: specifies the angle for drawing the rectangle
//  color	    	: specifies the Color to use for Fill the Rectangle.
//======================================================================
FLASHMEM void FlexIO2VGA::drawQuad(int16_t centerx, int16_t centery, int16_t w, int16_t h, int16_t angle, uint8_t color){
	int16_t	px[4],py[4];
	float	l;
	float	raddeg = 3.14159 / 180;
	float	w2 = w / 2.0;
	float	h2 = h / 2.0;
	float	vec = (w2*w2)+(h2*h2);
	float	w2l;
	float	pangle[4];

	l = sqrtf(vec);
	w2l = w2 / l;
	pangle[0] = acosf(w2l) / raddeg;
	pangle[1] = 180.0 - (acosf(w2l) / raddeg);
	pangle[2] = 180.0 + (acosf(w2l) / raddeg);
	pangle[3] = 360.0 - (acosf(w2l) / raddeg);
	px[0] = (int16_t)(calcco[((int16_t)(pangle[0]) + angle) % 360] * l + centerx);
	py[0] = (int16_t)(calcsi[((int16_t)(pangle[0]) + angle) % 360] * l + centery);
	px[1] = (int16_t)(calcco[((int16_t)(pangle[1]) + angle) % 360] * l + centerx);
	py[1] = (int16_t)(calcsi[((int16_t)(pangle[1]) + angle) % 360] * l + centery);
	px[2] = (int16_t)(calcco[((int16_t)(pangle[2]) + angle) % 360] * l + centerx);
	py[2] = (int16_t)(calcsi[((int16_t)(pangle[2]) + angle) % 360] * l + centery);
	px[3] = (int16_t)(calcco[((int16_t)(pangle[3]) + angle) % 360] * l + centerx);
	py[3] = (int16_t)(calcsi[((int16_t)(pangle[3]) + angle) % 360] * l + centery);
	// here we draw the quad
	drawLine(px[0],py[0],px[1],py[1],color, false);
	drawLine(px[1],py[1],px[2],py[2],color, false);
	drawLine(px[2],py[2],px[3],py[3],color, false);
	drawLine(px[3],py[3],px[0],py[0],color, false);
}
  
//======================================================================
//  Displays a filled Rectangle at a given Angle.
//  centerx			: specifies the center of the Rectangle.
//	centery
//  w,h 	        : specifies the size of the Rectangle.
//	angle			: specifies the angle for drawing the rectangle
//  fillcolor    	: specifies the Color to use for Fill the Rectangle.
//======================================================================
FLASHMEM void FlexIO2VGA::fillQuad(int16_t centerx, int16_t centery, int16_t w, int16_t h, int16_t angle, uint8_t fillcolor){
	int16_t	px[4],py[4];
	float	l;
	float	raddeg = 3.14159 / 180;
	float	w2 = w / 2.0;
	float	h2 = h / 2.0;
	float	vec = (w2*w2)+(h2*h2);
	float	w2l;
	float	pangle[4];

	l = sqrtf(vec);
	w2l = w2 / l;
	pangle[0] = acosf(w2l) / raddeg;
	pangle[1] = 180.0 - (acosf(w2l) / raddeg);
	pangle[2] = 180.0 + (acosf(w2l) / raddeg);
	pangle[3] = 360.0 - (acosf(w2l) / raddeg);
	px[0] = (int16_t)(calcco[((int16_t)(pangle[0]) + angle) % 360] * l + centerx);
	py[0] = (int16_t)(calcsi[((int16_t)(pangle[0]) + angle) % 360] * l + centery);
	px[1] = (int16_t)(calcco[((int16_t)(pangle[1]) + angle) % 360] * l + centerx);
	py[1] = (int16_t)(calcsi[((int16_t)(pangle[1]) + angle) % 360] * l + centery);
	px[2] = (int16_t)(calcco[((int16_t)(pangle[2]) + angle) % 360] * l + centerx);
	py[2] = (int16_t)(calcsi[((int16_t)(pangle[2]) + angle) % 360] * l + centery);
	px[3] = (int16_t)(calcco[((int16_t)(pangle[3]) + angle) % 360] * l + centerx);
	py[3] = (int16_t)(calcsi[((int16_t)(pangle[3]) + angle) % 360] * l + centery);
	// We draw 2 filled triangle for made the quad
	// To be uniform we have to use only the Fillcolor
	fillTriangle(px[0],py[0],px[1],py[1],px[2],py[2],fillcolor);
	fillTriangle(px[2],py[2],px[3],py[3],px[0],py[0],fillcolor);
	// here we draw the BorderColor from the quad
//	drawLine(px[0],py[0],px[1],py[1],bordercolor);
//	drawLine(px[1],py[1],px[2],py[2],bordercolor);
//	drawLine(px[2],py[2],px[3],py[3],bordercolor);
//	drawLine(px[3],py[3],px[0],py[0],bordercolor);
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

//==================================================================
// Draw arc.
// xcenter, ycenter = center of arc.
// xradius, yradius = major and minor radius.
// startAngle, endAngle = (0 to 360) positive and negative integers.
//==================================================================
FLASHMEM void FlexIO2VGA::drawArc(int xcenter, int ycenter,
                                  int xradius, int yradius,
                                  int startAngle, int endAngle) {
  float ang = (((startAngle<=endAngle) ? startAngle : endAngle) * (PI/180));
  float range = (((endAngle>startAngle) ? endAngle : startAngle) * (PI/180));
  float x = (xradius * cos(ang));
  float y = (yradius * sin(ang));
  do {
    vga4bit.drawPixel((int)(xcenter+x+0.5),(int)(ycenter-y+0.5),15);
    ang += 0.001;
    x = (xradius * cos(ang));
    y = (yradius * sin(ang));
  } while(ang <= range);
}

//==================================================================
// Displays an Ellipse.
// cx: specifies the X position
// cy: specifies the Y position
// radius1: minor radius of ellipse.
// radius2: major radius of ellipse.
// color: specifies the Color to use for draw the Border from the Ellipse.
//==================================================================
FLASHMEM void FlexIO2VGA::drawEllipse(int16_t cx, int16_t cy, int16_t radius1, int16_t radius2, uint8_t color){
  int x = -radius1, y = 0, err = 2-2*radius1, e2;
  float K = 0, rad1 = 0, rad2 = 0;

  rad1 = radius1;
  rad2 = radius2;

  if (radius1 > radius2)
  {
    do {
      K = (float)(rad1/rad2);
      drawPixel(cx-x,cy+(uint16_t)(y/K),color);
      drawPixel(cx+x,cy+(uint16_t)(y/K),color);
      drawPixel(cx+x,cy-(uint16_t)(y/K),color);
      drawPixel(cx-x,cy-(uint16_t)(y/K),color);

      e2 = err;
      if (e2 <= y) {
        err += ++y*2+1;
        if (-x == y && e2 <= x) e2 = 0;
      }
      if (e2 > x) err += ++x*2+1;
    }
    while (x <= 0);
  }
  else
  {
    y = -radius2;
    x = 0;
    do {
      K = (float)(rad2/rad1);
      drawPixel(cx-(uint16_t)(x/K),cy+y,color);
      drawPixel(cx+(uint16_t)(x/K),cy+y,color);
      drawPixel(cx+(uint16_t)(x/K),cy-y,color);
      drawPixel(cx-(uint16_t)(x/K),cy-y,color);

      e2 = err;
      if (e2 <= x) {
        err += ++x*2+1;
        if (-y == x && e2 <= y) e2 = 0;
      }
      if (e2 > y) err += ++y*2+1;
    }
    while (y <= 0);
  }
}

//==================================================================
// Draw a filled ellipse.
// cx: specifies the X position
// cy: specifies the Y position
// radius1: minor radius of ellipse.
// radius2: major radius of ellipse.
// fillcolor  : specifies the Color to use for Fill the Ellipse.
//==================================================================
FLASHMEM void FlexIO2VGA::fillEllipse(int16_t cx, int16_t cy, int16_t radius1, int16_t radius2, uint8_t fillcolor){
  int x = -radius1, y = 0, err = 2-2*radius1, e2;
  float K = 0, rad1 = 0, rad2 = 0;

  rad1 = radius1;
  rad2 = radius2;

  if (radius1 > radius2)
  {
    do {
      K = (float)(rad1/rad2);
      draw_v_line((cx+x), (cy-(uint16_t)(y/K)), (2*(uint16_t)(y/K) + 1) , fillcolor);
      draw_v_line((cx-x), (cy-(uint16_t)(y/K)), (2*(uint16_t)(y/K) + 1) , fillcolor);

      e2 = err;
      if (e2 <= y)
      {
        err += ++y*2+1;
        if (-x == y && e2 <= x) e2 = 0;
      }
      if (e2 > x) err += ++x*2+1;

    }
    while (x <= 0);
  }
  else
  {
    y = -radius2;
    x = 0;
    do {
      K = (float)(rad2/rad1);
      draw_h_line((cx-(uint16_t)(x/K)), (cy+y), (2*(uint16_t)(x/K) + 1) , fillcolor);
      draw_h_line((cx-(uint16_t)(x/K)), (cy-y), (2*(uint16_t)(x/K) + 1) , fillcolor);

      e2 = err;
      if (e2 <= x)
      {
        err += ++x*2+1;
        if (-y == x && e2 <= y) e2 = 0;
      }
      if (e2 > y) err += ++y*2+1;
    }
    while (y <= 0);
  }
}

FLASHMEM void FlexIO2VGA::drawRrect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t r, uint8_t color) {
  int16_t x = 0, y = 0;
  int16_t dx = 0, dy = 0;
  int16_t error = 0;
  
  if(x1 > x2) { x = x1; x1 = x2; x2 = x; }
  if(y1 > y2) { y = y1; y1 = y2; y2 = y; }
  r = min((r | 0), min((x2-x1)/2, (y2-y1)/2));

  uint16_t cx1 = x1 + r;
  uint16_t cx2 = x2 - r;
  uint16_t cy1 = y1 + r;
  uint16_t cy2 = y2 - r;


  vga4bit.drawLine(cx1, y1+1, cx2, y1+1, color, false);  //Top horz line.
  vga4bit.drawLine(cx1, y2-1, cx2, y2-1, color, false);  //Bottom horz line.
  vga4bit.drawLine(x1+1, cy1, x1+1, cy2, color, false);  //Left vert line. 
  vga4bit.drawLine(x2-1, cy1, x2-1, cy2, color, false);  //Right vert line.
  
  x = r;
  while(y <= x) {
    dy = 1 +2 * y;
    y++;
    error -= dy;
    if(error < 0) {
      dx = 1 - 2 * x;
      x--;
      error -= dx;
    }
    vga4bit.drawPixel(cx1 - x, cy1 - y, color);
    vga4bit.drawPixel(cx1 - y, cy1 - x, color);
    vga4bit.drawPixel(cx2 + x, cy1 - y, color);
    vga4bit.drawPixel(cx2 + y, cy1 - x, color);
    vga4bit.drawPixel(cx2 + x, cy2 + y, color);
    vga4bit.drawPixel(cx2 + y, cy2 + x, color);
    vga4bit.drawPixel(cx1 - x, cy2 + y, color);
    vga4bit.drawPixel(cx1 - y, cy2 + x, color);
  }
}

FLASHMEM void FlexIO2VGA::fillRrect(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t r, uint8_t color) {
  int16_t x = 0, y = 0;
  int16_t dx = 0, dy = 0;
  int16_t error = 0;
  
  if(x1 > x2) { x = x1; x1 = x2; x2 = x; }
  if(y1 > y2) { y = y1; y1 = y2; y2 = y; }
  r = min((r | 0), min((x2-x1)/2, (y2-y1)/2));

  uint16_t cx1 = x1 + r;
  uint16_t cx2 = x2 - r;
  uint16_t cy1 = y1 + r;
  uint16_t cy2 = y2 - r;

//  vga4bit.fbUpdate(true);
  vga4bit.fillRect(x1,cy1,x2,cy2,color);

//  vga4bit.fbUpdate(true);
  x = r;
  while(y <= x) {
    dy = 1 +2 * y;
    y++;
    error -= dy;
    if(error < 0) {
      dx = 1 - 2 * x;
      x--;
      error -= dx;
    }
    vga4bit.drawLine(cx1-x, cy1-y, cx2+x, cy1-y, color, false);
    vga4bit.drawLine(cx1-y, cy1-x, cx2+y, cy1-x, color, false);
    vga4bit.drawLine(cx1-x, cy2+y, cx2+x, cy2+y, color, false);
    vga4bit.drawLine(cx1-y, cy2+x, cx2+y, cy2+x, color, false);
  }
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
    // copy from last line FIXED for scroll down
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
      // copy from last line pixel FIXED for scroll left and right.
      sxpos = c_s_x + c_w;// - 1;
      dxpos = c_d_x + c_w;// - 1;
      dx = -1;
  } else {
    // copy from first line pixel
    sxpos = c_s_x;
    dxpos = c_d_x;
    dx = 1;
  }
  
  // Do copy...
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
  // Initialize text and cursor settings to default.
  init_text_settings();
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

  memcpy(currentFont,font_8x16,sizeof(font_8x16));

  promp_size = 0;  
  print_window_x = 0;
  print_window_y = 0;
  print_window_w = fb_width / font_width / (double_width ? 2:1);
  print_window_h = fb_height / font_height / (double_height ? 2:1);

  foreground_color = VGA_BRIGHT_WHITE;
  background_color = VGA_BLUE;
  SaveRVFGC = VGA_BRIGHT_WHITE;
  SaveRVBGC = VGA_BLUE;
  transparent_background = false;
  clear(background_color);

  tCursor.tCursor_x = 0;
  tCursor.tCursor_y = 0;
  tCursor.active = false;
  vga4bit.initCursor(1,0,7,7,true,30); 
  vga4bit.initGcursor(0,1,0,8,8);
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
// Get a character under next mouse pointer position
// from frame buffer and save it:
// x = starting x position in frame buffer.
// y = starting y position in frame buffer.
// font_width / 2 (2 pixels per byte).
// font_height * 2 (compensate for font_width / 2).
// Note: Mouse pointer size is always fixed at 8x16.
//====================================================
FLASHMEM void FlexIO2VGA::getGptr(int16_t x, int16_t y, uint8_t *buf) {
  for(int16_t i = 0; i < font_height*2+1; i++) {
    for(int16_t j = 0; j < (font_width/2)+1; j++) {
      *buf++ = getByte((x/2)+j, y+i);
    }
  }
}

//====================================================
// Draw a graphic pointer to frame buffer and replace
// with previously saved character (with getGptr()) at 
// previous pointer position:
// x = starting x position in frame buffer.
// y = starting y position in frame buffer.
// font_width / 2 (2 pixels per byte).
// font_height * 2 (compensate for font_width / 2).
// Note: Mouse pointer size is always fixed at 8x16.
//====================================================
FLASHMEM void FlexIO2VGA::putGptr(int16_t x, int16_t y, uint8_t *buf) {
  for(int16_t i = 0; i < font_height*2+1; i++) {
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
  print_window_h = fb_height / font_height;
  setBlkCursorDims(tCursor.x_start,tCursor.y_start,tCursor.x_end,font_height,0);
  if(runflag == true) clear(background_color);
  return (int)fsize;
}

//======================================================
// Load a font from memory to current char array.
// font: font is a pointer to a char array (4096 bytes).
//======================================================
FLASHMEM int FlexIO2VGA::fontLoadMem(uint8_t *font) {

  memcpy(currentFont,font,sizeof(currentFont));
  return (int)0;
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
//      charPointer = &font_8x16[t*font_height];
      charPointer = &currentFont[t*font_height]; // currentFont[] is a loadable font buffer.
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

//==========================================
// Define text printing window.
// x, y, width and height are in characters.
//==========================================
FLASHMEM void FlexIO2VGA::setPrintCWindow(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
  if(x < 0) {
    print_window_x = 0;
  } else if(x >= ((fb_width / font_width) - font_width)) {
    print_window_x = (fb_width / font_width);
  } else {
    print_window_x = (x * font_width);
  }
  
  if(y < 0) {
    print_window_y = 0;
  } else if(y >= ((fb_height / font_height) - font_height)) {
    print_window_y = (fb_height / font_height);
  } else {
    print_window_y = (y *font_height);
  }

  print_window_w = width / (double_width ? 2:1);
  print_window_h = height / (double_height ? 2:1);
  textxy(0,0);
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
  //=====================================================
  // Clear the character print window in 800x600 mode
  // with a font height of 16 needs fb_height adjusted
  // to 38 characters as 600 / (font_height == 16) = 37.5
  // character lines. which returns an int of 37. So we
  // set fb_height to 38 so complete screen is cleared.
  // A font_height of 8 gives an even 75 character lines.
  // The following two lines set the proper
  //======================================================
  uint8_t adjust = 0;
  if(((fb_height / font_height) == 37/*37.5*/) && (font_height == 16)) {
    adjust = 38; // Adjust if needed for 800x600, fontsize 16 full screen.
  } else {
    adjust = font_height; // Normal defined print window less than 800x600.
  }

  fillRect(print_window_x, print_window_y, print_window_x +
          (print_window_w) * font_width, print_window_y +
          (print_window_h) * adjust, background_color);
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
// Scroll up text one row. (Slow!)
// Set font_height to "-font_height" (negative)
// to scroll up one row.
//=========================================
FLASHMEM void FlexIO2VGA::scrollUpPrintWindow() {
  // move the 2nd row and the following ones one row up
  Vscroll(print_window_x, print_window_y + font_height, 
  (print_window_w) * font_width * (double_width ? 2:1), (print_window_h - 1) *
  font_height, -font_height, background_color);
}

//=========================================
// Scroll up text one line. (Slow!)
// Set font_height to "font_height" (positive)
// to scroll down one line.
//=========================================
FLASHMEM void FlexIO2VGA::scrollDownPrintWindow() {
  // move the 2nd line and the following ones one line down
  Vscroll(print_window_x, print_window_y, 
  (print_window_w) * font_width, (print_window_h - 1) * font_height,
  font_height, background_color);
}

//=============================================
// Scroll right text one column. (Slow!)
// Set font_width to "-font_width" (negative)
// to scroll right one column.
//=============================================
FLASHMEM void FlexIO2VGA::scrollRightPrintWindow() {
  // move the 2nd column and the following ones one column right
  Hscroll(print_window_x, print_window_y,// + font_height,
  (print_window_w) * font_width, (print_window_h) * font_height,
  font_width, background_color);
}

//============================================
// Scroll left text one column. (Slow!)
// Set font_width to "font_width" (positive)
// to scroll left one column.
//============================================
FLASHMEM void FlexIO2VGA::scrollLeftPrintWindow() {
  // move the 2nd column and the following ones one column left
  Hscroll(print_window_x, print_window_y,// + font_height,
  (print_window_w) * font_width, (print_window_h) * font_height,
  -font_width, background_color);
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
FLASHMEM void FlexIO2VGA::drawTcursor(int color) {
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
  tCursorOff();
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
  if(isActive) tCursorOn();
}

//================================================
// Draw graphic cursor at current character position.
// Cursor size vertically and horizontally are
// based on font sizes 8x8 or 8x16. (8x16 max)
// Fixed mouse pointer trails issue in 8x8 mode. 
//================================================
FLASHMEM void FlexIO2VGA::drawGcursor(int color) {
  if(gCursor.active) {
    getGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
    if(gCursor.type == BLOCK_CURSOR) {
      fillRect(gCursor.gCursor_x+gCursor.x_start, gCursor.gCursor_y+gCursor.y_start,
               gCursor.gCursor_x+gCursor.x_end-1, gCursor.gCursor_y+gCursor.y_end-1,
               gCursor.color);
    } else {
      drawBitmap(gCursor.gCursor_x, gCursor.gCursor_y, (uint8_t *)arrow[gCursor.type], 8, 16);
    }
  }
}

//=============================================
// Move graphic cursor position to column/line.
//=============================================
FLASHMEM void FlexIO2VGA::moveGcursor(int16_t column, int16_t line) {
  bool isActive = false;
  if(gCursor.active) {
    gCursorOff();
    isActive = true;
    putGptr(gCursor.gCursor_x,gCursor.gCursor_y,gCursor.char_under_cursor);
    gCursor.gCursor_x = column;
    gCursor.gCursor_y = line;
    drawGcursor(foreground_color);
  }
  if(isActive) gCursorOn();
}

//======================================================================
// Scroll an area of the screen, top left corner (x,y),
// width w, height h by (dx,dy) pixels. If dx>0 scrolling is right, dx<0
// is left. dy>0 is down, dy<0 is up.
// Empty area is filled with color col.
// (only when horizontal (dy=0) or vertical scroll (dx=0))
//======================================================================
FLASHMEM void FlexIO2VGA::scroll(int x, int y, int w, int h, int dx, int dy,int col) {
  if(dy == 0)	{
    if(dx == 0) return; // If dx and dy == 0 then error. Return.
    Hscroll(x, y, w, h, dx, col);
  }	else if(dx == 0) {
    Vscroll(x, y, w, h, dy, col);
  }	else {
    copy(x, y, x + dx, y + dy, w, h);
  }
}

//======================================================================
// Hscroll used by scroll(). Scrolls right or left. Fixed 06-11-25
// dx < 0 = scroll left. dy >= 0 = scroll right.
//======================================================================
void FlexIO2VGA::Hscroll(int x, int y, int w, int h, int dx, int col)
{
  // Copy print window left or right 1 position.
  copy(x, y, x + dx, y, w, h);

  // fill empty area created with col (background color).
  if(dx > 0) {
    // move to the right => fill area on the left side of source area with col.
    fillRect(x, y, x + dx , y + h - 1, col); // this. FIXED 
  }	else {
    // move to the left => fill area on the right side of source area with col.
    fillRect(x + w + dx, y, x + w, y + h, col); // this. FIXED
  }
}

//======================================================================
// Vscroll used by scroll(). Scrolls up or down. 
// dy < 0 = scroll up. dy >= 0 = scroll down.
//======================================================================
inline void FlexIO2VGA::Vscroll(int x, int y, int w, int h, int dy, int col) {
  // Copy print window up or down 1 position.
  copy(x, y, x, y + dy, w, h);
  // fill empty area created with col (background color).
  if(dy > 0) {
    // move to the bottom => fill area on the top side of source area
    fillRect(x, y, x + w - 1, y + dy - 1, col);
  }	else {
    // move to the top => fill area on the bottom side of source are
    fillRect(x, y + h + dy, x + w - 1, y + h - 1, col);
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
  bool isGCActive = false;
    
  // If text cursor is active, set flag and turn off cursor.
  if(tCursor.active) {
    tCursorOff();
    isActive = true;
  }
  // If graphic cursor is active, set flag and turn off cursor.
  if(tCursor.active) {
    gCursorOff();
    isGCActive = true;
  }
  switch(c) {
    case '\r':
//      cursor_x = 0;  ignore carriage return.
      break;
    case '\n': // Proccess linefeed.
      cursor_x = 0;
      cursor_y ++;
      if(cursor_y >= (print_window_h /*/ (double_height ? 2:1)*/)) {
        scrollUpPrintWindow(); // Scroll up one line.
        cursor_y = (print_window_h - 1);// / (double_height ? 2:1);
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
      if(cursor_x >= (print_window_w * (double_width ? 2:1))) write('\n'); // Do linefeed.
      buf[0] = c;
      buf[1] = '\0';
      drawText(print_window_x + cursor_x * font_width,
               print_window_y + cursor_y * font_height, buf, 
               foreground_color, background_color, VGA_DIR_RIGHT);
      cursor_x++;
  }
  updateTCursor(cursor_x, cursor_y);
  if(isActive) tCursorOn();
  if(isGCActive) gCursorOn();
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

//===========================================
// Clear the status line. The 800x600 mode
// needs fb_height adjusted to 600 - 8 as
// 600 / (font_height == 16) = 37.5 character
// lines. We subtract 8 from fb_height to get
// an even 37 character lines. A font_height
// of 8 gives an even 75 character lines.
//===========================================
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
FLASHMEM void FlexIO2VGA::clreol(void) {
  int16_t tempX = cursor_x;
  int16_t tempY = cursor_y;
  bool isActive = false;
  if(tCursor.active) isActive = true;

  for(int i = 0; i < (print_window_w-tempX); i++) {
	write(0x20);
    tCursorOff();
  }
  textxy(tempX,tempY);
  if(isActive) tCursorOn();

}

//====================================================
// Support function for VT100: Clear to End Of Screen.
//====================================================
FLASHMEM void FlexIO2VGA::clreos(void) {
  int16_t tempX = cursor_x;
  int16_t tempY = cursor_y;

  bool isActive = false;
  if(tCursor.active) isActive = true;

  clreol();

  for(uint16_t y = 0; y < (print_window_h - tempY); y++)
    for(int16_t x = 0; x < (print_window_w-tempX); x++)
      write(0x20); 

  textxy(tempX,tempY);
  if(isActive) tCursorOn();
}

//========================================================
// Support function for VT100: Clear to beginning of line.
//========================================================
FLASHMEM void FlexIO2VGA::clrbol(void) {
  int16_t tempX = cursor_x;
  int16_t tempY = cursor_y;

  bool isActive = false;
  if(tCursor.active) isActive = true;

  for(int16_t x = tempX; x > 0; x--) write(0x7f);

  textxy(tempX,tempY);
  if(isActive) tCursorOn();
}

//=========================================================
// Support function for VT100: Clear to begining of Screen.
//=========================================================
FLASHMEM void FlexIO2VGA::clrbos(void) {
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
  if(isActive) tCursorOn();
}

//========================================
// Support function for VT100: Clear Line.
//========================================
FLASHMEM void FlexIO2VGA::clrlin(void) {
  int16_t tempX = cursor_x;
  int16_t tempY = cursor_y;
  bool isActive = false;
  if(tCursor.active) isActive = true;

  textxy(0,tempY);
  for(int16_t x = 0; x < print_window_w; x++) write(0x20);

  textxy(tempX,tempY);
  if(isActive) tCursorOn();
}

//=========================================
// Reverse forground and background colors.
//=========================================
FLASHMEM void FlexIO2VGA::reverseVid(bool onOff) {
  getChar(tCursorX(),tCursorY(),tCursor.char_under_cursor);
  if(onOff) {
    SaveRVFGC = vga4bit.getTextFGC();
    SaveRVBGC = vga4bit.getTextBGC();
    vga4bit.textColor(SaveRVBGC, SaveRVFGC);
  } else {
    vga4bit.textColor(SaveRVFGC, SaveRVBGC);
  }
}

//--------------------------------------------------------------
//  Displays a Polygon.
//  centerx			: are specified with PolySet.Center.x and y.
//	centery
//  cx              : Translate the polygon in x direction
//  cy              : Translate the polygon in y direction
//  bordercolor  	: specifies the Color to use for draw the Border from the polygon.
//  polygon points  : are specified with PolySet.Pts[n].x and y 
//  After the last polygon point , set PolySet.Pts[n + 1].x to 10000
//  Max number of point for the polygon is set by MaxPolyPoint previously defined.
//--------------------------------------------------------------
FLASHMEM void FlexIO2VGA::drawpolygon(int16_t cx, int16_t cy, uint8_t bordercolor){
	uint8_t n = 1;
	while((PolySet.Pts[n].x < 10000) && (n < MaxPolyPoint)){
		drawLine(PolySet.Pts[n].x + cx, 
	             PolySet.Pts[n].y + cy, 
				 PolySet.Pts[n - 1].x + cx , 
				 PolySet.Pts[n - 1].y + cy, 
				 bordercolor, false);
		n++;		
	}
	// close the polygon
	drawLine(PolySet.Pts[0].x + cx, 
	         PolySet.Pts[0].y + cy, 
			 PolySet.Pts[n - 1].x + cx, 
			 PolySet.Pts[n - 1].y + cy, 
			 bordercolor, false);
}

//--------------------------------------------------------------
//  Displays a filled Polygon.
//  centerx			: are specified with PolySet.Center.x and y.
//	centery
//  cx              : Translate the polygon in x direction
//  cy              : Translate the polygon in y direction
//  fillcolor  	    : specifies the Color to use for filling the polygon.
//  bordercolor  	: specifies the Color to use for draw the Border from the polygon.
//  polygon points  : are specified with PolySet.Pts[n].x and y 
//  After the last polygon point , set PolySet.Pts[n + 1].x to 10000
//  Max number of point for the polygon is set by MaxPolyPoint previously defined.
//--------------------------------------------------------------
FLASHMEM void FlexIO2VGA::drawfullpolygon(int16_t cx, int16_t cy, uint8_t fillcolor, uint8_t bordercolor){
	int n,i,j,k,dy,dx;
	int y,temp;
	int a[MaxPolyPoint][2],xi[MaxPolyPoint];
	float slope[MaxPolyPoint];

    n = 0;

	while((PolySet.Pts[n].x < 10000) && (n < MaxPolyPoint)){
		a[n][0] = PolySet.Pts[n].x;
		a[n][1] = PolySet.Pts[n].y;
		n++;
	}

	a[n][0]=PolySet.Pts[0].x;
	a[n][1]=PolySet.Pts[0].y;

	for(i=0;i<n;i++)
	{
		dy=a[i+1][1]-a[i][1];
		dx=a[i+1][0]-a[i][0];

		if(dy==0) slope[i]=1.0;
		if(dx==0) slope[i]=0.0;

		if((dy!=0)&&(dx!=0)) /*- calculate inverse slope -*/
		{
			slope[i]=(float) dx/dy;
		}
	}

	for(y=0;y< 480;y++)
	{
		k=0;
		for(i=0;i<n;i++)
		{

			if( ((a[i][1]<=y)&&(a[i+1][1]>y))||
					((a[i][1]>y)&&(a[i+1][1]<=y)))
			{
				xi[k]=(int)(a[i][0]+slope[i]*(y-a[i][1]));
				k++;
			}
		}

		for(j=0;j<k-1;j++) /*- Arrange x-intersections in order -*/
			for(i=0;i<k-1;i++)
			{
				if(xi[i]>xi[i+1])
				{
					temp=xi[i];
					xi[i]=xi[i+1];
					xi[i+1]=temp;
				}
			}

		for(i=0;i<k;i+=2)
		{
			drawLine(xi[i] + cx,y + cy,xi[i+1]+1 + cx,y + cy, fillcolor, false);
		}

	}

	// Draw the polygon outline
	drawpolygon(cx , cy , bordercolor);
}

//--------------------------------------------------------------
//  Displays a rotated Polygon.
//  centerx			: are specified with PolySet.Center.x and y.
//	centery
//  cx              : Translate the polygon in x direction
//  ct              : Translate the polygon in y direction
//  bordercolor  	: specifies the Color to use for draw the Border from the polygon.
//  polygon points  : are specified with PolySet.Pts[n].x and y 
//  After the last polygon point , set PolySet.Pts[n + 1].x to 10000
//  Max number of point for the polygon is set by MaxPolyPoint previously defined.
//--------------------------------------------------------------
FLASHMEM void FlexIO2VGA::drawrotatepolygon(int16_t cx, int16_t cy, int16_t Angle, uint8_t fillcolor, uint8_t bordercolor, uint8_t filled)
{
	Point2D 	SavePts[MaxPolyPoint];
	uint16_t	n = 0;
	int16_t		ctx,cty;
	float		raddeg = 3.14159 / 180;
	float		angletmp;
	float		tosquare;
	float		ptsdist;

	ctx = PolySet.Center.x;
	cty = PolySet.Center.y;
	
	while((PolySet.Pts[n].x < 10000) && (n < MaxPolyPoint)){
		// Save Original points coordinates
		SavePts[n] = PolySet.Pts[n];
		// Rotate and save all points
		tosquare = ((PolySet.Pts[n].x - ctx) * (PolySet.Pts[n].x - ctx)) + ((PolySet.Pts[n].y - cty) * (PolySet.Pts[n].y - cty));
		ptsdist  = sqrtf(tosquare);
		angletmp = atan2f(PolySet.Pts[n].y - cty,PolySet.Pts[n].x - ctx) / raddeg;
		PolySet.Pts[n].x = (int16_t)((cosf((angletmp + Angle) * raddeg) * ptsdist) + ctx);
		PolySet.Pts[n].y = (int16_t)((sinf((angletmp + Angle) * raddeg) * ptsdist) + cty);
		n++;
	}	
	
	if(filled != 0)
	  drawfullpolygon(cx , cy , fillcolor , bordercolor);
    else
	  drawpolygon(cx , cy , bordercolor);

	// Get the original points back;
	n=0;
	while((PolySet.Pts[n].x < 10000) && (n < MaxPolyPoint)){
		PolySet.Pts[n] = SavePts[n];
		n++;
	}	
}

//==========================================================================================
//= The following Graphic Button functions are based on Adafruits Graphic button libraries =
//= and adapted for use with Teensy and VGA_4bit_T4 library.                               =
//==========================================================================================
// Initialize a graphic button. 
FLASHMEM void FlexIO2VGA::initButton(struct Gbuttons *buttons, int16_t x, int16_t y, int16_t w, int16_t h,
 uint8_t outline, uint8_t fill, uint8_t textcolor,
 char *label, uint8_t textsize)
{
	buttons->x = x;
	buttons->y = y;
	buttons->w = w;
	buttons->h = h;
	buttons->outlinecolor = outline;
	buttons->fillcolor = fill;
	buttons->textcolor = textcolor;
	buttons->textsize  = textsize;
	strncpy(buttons->label, label, 9);
	buttons->label[9] = 0;
}

// Draw graphic button. (not completely implemented yet)
FLASHMEM void FlexIO2VGA::drawButton(struct Gbuttons *buttons, boolean inverted) {
  uint16_t fill, outline;
//  uint16_t text;
  uint16_t fgcolor = foreground_color;	
  uint16_t bgcolor = background_color;	

  if(!inverted) {
    fill    = buttons->fillcolor;
    outline = buttons->outlinecolor;
//    text    = buttons->textcolor;
  } else {
    fill    = buttons->textcolor;
    outline = buttons->outlinecolor;
//    text    = buttons->fillcolor;
  }
  vga4bit.fillRrect(buttons->x, buttons->y, buttons->w+buttons->x, buttons->h+buttons->y,min(buttons->w,buttons->h), fill);
  vga4bit.drawRrect(buttons->x-1, buttons->y-1, (buttons->w+1)+buttons->x, (buttons->h+1)+buttons->y,min(buttons->w,buttons->h), outline);

//  ->setCursor(_x - strlen(_label)*3*_textsize, _y-4*_textsize);
//  ->setTextColor(text);
//  ->setTextSize(_textsize);
//  ->print(_label);
  textColor(fgcolor,bgcolor);	// restore colors	
}

// Check the button to see if it is within the range of selection.
FLASHMEM bool FlexIO2VGA::buttonContains(struct Gbuttons *buttons, int16_t x, int16_t y) {
  if ((x < buttons->x) || (x > (buttons->x + buttons->w))) return false;
  if ((y < buttons->y) || (y > (buttons->y + buttons->h))) return false;
  return true;
}

// Signal state of buttons: true = pressed, false = released
FLASHMEM void FlexIO2VGA::buttonPress(struct Gbuttons *buttons, boolean p) {
  buttons->laststate = buttons->currstate;
  buttons->currstate = p;
}

// Check the current state of a button: pressed/released
FLASHMEM bool FlexIO2VGA::buttonIsPressed(struct Gbuttons *buttons) {
	return buttons->currstate;
}

// Check if button was just pressed
FLASHMEM bool FlexIO2VGA::buttonJustPressed(struct Gbuttons *buttons) {
	return (buttons->currstate && !buttons->laststate);
 }

// Check if button was just released
FLASHMEM bool FlexIO2VGA::buttonJustReleased(struct Gbuttons *buttons) {
	return (!buttons->currstate && buttons->laststate);
}
//==========================================================================================

extern FlexIO2VGA vga4bit;

FASTRUN void FlexIO2VGA::ISR(void) {
  uint32_t timStatus = FLEXIO2_TIMSTAT & 0xFF;
  FLEXIO2_TIMSTAT = timStatus;

  if (timStatus & (1<<5)) {
    vga4bit.TimerInterrupt();
  }

  asm volatile("dsb");
}
void vgadump(uint8_t *memory, uint16_t len) {
   	uint16_t	i=0, j=0;
	unsigned char	c=0;

//	printf("                     (FLASH) MEMORY CONTENTS");
	Serial.printf("\n\rADDR          00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F");
	Serial.printf("\n\r-------------------------------------------------------------\n\r");


	for(i = 0; i <= (len-1); i+=16) {
//		phex16((i + memory));
		Serial.printf("%8.8x",(unsigned int)(i + memory));
		Serial.printf("      ");
		for(j = 0; j < 16; j++) {
			c = memory[i+j];
			Serial.printf("%2.2x",c);
			Serial.printf(" ");
		}
		Serial.printf("  ");
		for(j = 0; j < 16; j++) {
			c = memory[i+j];
			if(c > 31 && c < 127)
				Serial.printf("%c",c);
			else
				Serial.printf(".");
		}
//		_delay_ms(10);
		Serial.printf("\n");
	}
}

/* END VGA driver code */
