# VGA_4BIT_T4

## This library is based on jmarsh's FlexIO2VGA sketch found here:

## https://forum.pjrc.com/threads/72710-VGA-output-via-FlexIO-(Teensy4)

## Most of the graphic and text functions were taken and adapted from uVGA found here:

## https://github.com/qix67/uVGA

# This library is WIP and is far from complete or correct!! Playing and testing.

## Update: Added KiCad fabrication files for a T4.1 VGA adaptor board. The zip file can be found in the Extras folder.

Schematic for RGBI R2R VGA ladder can be found at the begining of VGA_4bit_T4.cpp file.

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

Since this version of the driver is using DMA memory DMA and malloc memory are affected by the screen size. The VGA_4bit_Config.h file contains the maximum memory used. By default it is set for a maximum 800x600 screen. To use a 1024x768 screen you will need to change the config fille for 1024x768 otherwise you run the risk of trashing the stack and variables.

Each nibble (one pixel) can be one of 16 RGBI colors (0 to 15):
-  0 VGA_BLACK
-  1 VGA_BLUE
-  2 VGA_GREEN
-  3 VGA_CYAN
-  4 VGA_RED
-  5 VGA_MAGENTA
-  6 VGA_YELLOW
-  7 VGA_WHITE
-  8 VGA_GREY
-  9 VGA_BRIGHT_BLUE
- 10 VGA_BRIGHT_GREEN
- 11 VGA_BRIGHT_CYAN
- 12 VGA_BRIGHT_RED
- 13 VGA_BRIGHT_MAGENTA
- 14 VGA_BRIGHT_YELLOW
- 15 VGA_BRIGHT_WHITE

There a few examples of usage including a graphic cursor as well as a simple screen editor using an adapted version of the Kilo editor found online here: https://viewsourcecode.org/snaptoken/kilo/.

### Examples:
- VGA_T4_Editor -- Requires a USB keyboard plugged into the T4.1 USB host connector. The Kilo editor supports most common editor keys, simple syntax highlighting of C/CPP/BAS files, simple searching and horizontal scrolling of long lines. This sketch uses built in SD card reader and optionally a USB drive if using a USB hub. Uncomment "#define USE_USB_DRIVE 1" at begining of sketch to use USB drive. F1 key for help.

- VGA_T4_Gauges  -- This is a version of Sumotoy's gauges sketch found in his RA8875 library.

- VGA_T4_GraphicCursor -- An example of a software driven graphic cursor. Requires a USB mouse connected to the T4.1 USB host connector. There are four types of graphic cursors available (Block, Filled Arrow, Hollow Arrow and I-beam). This uses the "drawBitmap()" function to render the cursor. More could be added. Uses 8x16 bitmap.

- Grahics -- Demonstrates some of the graphic primitives like drawing circles, filled circles, triangles, filled triangles, rectangles, filled rectangles, ellipses and filled ellipses. 

- VGA_T4_Mandelbrot -- A uVGA example adapted to VGA_4BIT_T4.

- VGA_T4_treeDee -- Another Sumotoy example showing a rotating 3D framed cube.

- VGA_T4_Box_Test -- Demonstrates allocating memory for saving and recalling sections of video memory. Basis for windowing functions.

- VGA_T4_Windows_Test -- A very simple windowing system.

- Double_Size_Font_Test.ino -- Demonstrates screen double width and double height font modes.

- VGA_T4_Print_Window_demo.ino -- Demonstrates setting up a print window framed by a graphic double line box and scrolling text within the boundries of the print window.

- VGA_T4_Text.ino -- Displays a couple of text screens repeatedly.

- Loadable_Font_Demo.ino -- To use this sketch you will need to copy the four '.fnt' files found in the same directory as this sketch to an SD card or a USB drive. The file names are "bold.fnt", " italics.fnt", "" standard.fnt", " thin.fnt". The font files are 4096 bytes in size and represent 256x16 font array. One define in sketch used to switch between SD card or USB drive usage.

Filled ellipses have a problem with filling out of bounds with certain parameter combinations have not figured out why yet. Example of fail: fillEllipse(516, 419,59,61,VGA_BRIGHT_WHITE). Resolution does not matter.- More to come MAYBE...

All files are heavily commented.

### CHANGES:
- Finally drawing text in four different directions.

06-11-25
- VGA_T4_GraphicCursor.ino -- Single/Double click function disabled for now, it is blocking and needs rewrite. Also fixed mouse cursor trails when 8x8 font size is used.
- Fixed and updated scrolling in all four directions. UP, down, left and right. Will be adding a sketch demonstrating scrolling capabiites.
- VGA_T4_Windows_Test.ino -- Fixed several bugs and update windowing system.
- VGA_T4_Editor -- Fixed incomplete screen clear in 800x600 screen mode.

### Added sketches: 06-12-25
- VGA_T4_Print_Window_demo.ino -- Demonstrates setting up a print window of various dimensions. Also demonstrates using colors, printing and scrolling.
- VGA_T4_Scroll_Test.ino -- Demonstrates scrolling capabilities. Can scroll a print window in one of four directions, up, down, right and left. Also capable of scrolling graphics.

06-13-25
- VGA_T4_GraphicCursor -- Fix issue with graphic mouse cursor disappering under active text.

### Added sketch: 06-25-25
- Double_Size_Font_Test.ino -- see the above requirements for this sketch. (Requirements also noted in the sketch).
- Several more bug fixes.

### Added sketch: 06-25-25
- Loadable_Font_Demo.ino
- Updated source files for loadable fonts

### TODO:
- Filled ellipses have a problem with filling out of bounds with certain parameter combinations have not figured out why yet. Example of fail: fillEllipse(516, 419,59,61,VGA_BRIGHT_WHITE). Resolution does not matter.

### NOTES:
- Clear the character print window in 800x600 mode with a font height of 16 needs fb_height adjusted to 38 characters as 600 / (font_height == 16) = 37.5 character lines. which returns an   int of 37. So we set fb_height to 38 so complete screen is cleared. A font_height of 8 gives an even 75 character lines. This is to make sure the bottom scan lines are cleared.

## Again this is library is WIP and just for learning and playing with FlexIO2VGA capabilities...

