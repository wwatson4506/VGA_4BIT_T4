# VGA_4BIT_T4

## This library is based on jmarsh's FlexIO2VGA sketch found here:

## https://forum.pjrc.com/threads/72710-VGA-output-via-FlexIO-(Teensy4)

## Most of the graphic and text function were taken and adapted from uVGA found here:

## https://github.com/qix67/uVGA
 
# This library is WIP and is far from complete or correct!! Playing and testing.

This is a 4 bit per pixel VGA driver using FlexIO2VGA. 
There are three main files:
- VGA_4bit_T4.cpp
- VGA_4bit_T4.h
- VGA_4bit_Config.h

I wanted to optimize memory usage so at this time I am not using double buffering although it is available. The hard part was learning to pack and unpack two pixels in one byte. Once that was figured out it was fairly easy to build on it. Thanks for the help jmarsh...

There are four supported video modes:
- 1024x768x60
- 800x600x60
- 640x480x60
- 640x400x70

Each nibble (one pixel) can be one of 16 RGBI colors:
-  VGA_BLACK ------------> 0
-  VGA_BLUE -------------> 1
-  VGA_GRE --------------> 2
-  VGA_CYAN -------------> 3
-  VGA_RED --------------> 4
-  VGA_MAGENTA ----------> 5
-  VGA_YELLOW -----------> 6
-  VGA_WHITE ------------> 7
-  VGA_GREY -------------> 8
-  VGA_BRIGHT_BLUE ------> 9
-  VGA_BRIGHT_GREEN ----> 10
-  VGA_BRIGHT_CYAN -----> 11
-  VGA_BRIGHT_RED ------> 12
-  VGA_BRIGHT_MAGENTA --> 13
-  VGA_BRIGHT_YELLOW ---> 14
-  VGA_BRIGHT_WHITE ----->15


Since this version of the driver is using DMA memory DMA and malloc memory are affected by the screen size.



