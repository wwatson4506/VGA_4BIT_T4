//============================
// VGA_T4_Config.h
//============================
#ifndef _VGA_T4_CONFIG_H
#define _VGA_T4_CONFIG_H

//===============================================
// Uncomment the pair that represents the largest
// screen dimensions you will need.
// The larger the width and height dimensions the
// less DMAMEM/malloc memory will be available.
//===============================================

#define MAX_WIDTH (1024/2)
#define MAX_HEIGHT 768

//#define MAX_WIDTH (800/2)
//#define MAX_HEIGHT 600

//#define MAX_WIDTH (640/2)
//#define MAX_HEIGHT 480

//#define MAX_WIDTH (640/2)
//#define MAX_HEIGHT 400
//===============================================

#define TABSIZE 4

/************************************************
Supported timings:
  const vga_timing *timing = &t640x400x70;
  const vga_timing *timing = &t640x480x60;
  const vga_timing *timing = &t800x600x60;
  const vga_timing *timing = &t1024x600x60;
  const vga_timing *timing = &t1024x768x60;
Unsupported timings:
  const vga_timing *timing = &t1280x720x60;
*************************************************/
#endif
