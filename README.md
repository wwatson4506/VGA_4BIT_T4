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

I wanted to optimize memory usage so at this time I am not using double buffering although it is available. The hard part was learning to pack and unpack two pixels in one byte. Once that was figured out it was fairly easy to build on it.
