//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Loadable_Font_Demo.ino
//======================================================================
// To use this sketch you will need to copy the four '.fnt' files found
// in the same directory as this sketch to an SD card or a USB drive.
// The file names are:
//  - bold.fnt
//  - italics.fnt
//  - standard.fnt
//  - thin.fnt
// The font files are 4096 bytes in size and represent 256x16 font array.
// By default this sketch uses the SD card to load the font files. But
// you you can also use a USB stick as well by uncommenting the define
// below. ------------------------------------------------------------
#include "VGA_4bit_T4.h"//                                           |
//                                                                   |
//================================================================   |
// UnComment this define to use a USB drive instead of an SD card.   |
//#define USE_USB_DRV 1 <---------------------------------------------
//================================================================

#if defined(USE_USB_DRV)
#include <USBHost_t36.h>

// Setup USBHost_t36 and as many HUB ports as needed.
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
USBHub hub4(myusb);

// Setup MSC for the number of USB Drives you are using. (one for this
// example). Mutiple  USB drives can be used. Hot plugging is supported.
USBDrive myDrive(myusb);
USBFilesystem firstPartition(myusb);
#else
#include <SD.h>

const int chipSelect = BUILTIN_SDCARD;
#endif

#define FONTSIZE 16 // Only 8x16 loadable ffonts are supported at this time.

File myFile;

// Uncomment one of the following screen resolutions. Try them all:)
//const vga_timing *timing = &t1024x768x60;
const vga_timing *timing = &t800x600x60;
//const vga_timing *timing = &t640x480x60;
//const vga_timing *timing = &t640x400x70;

// Must use this instance name. It's used in the driver.
FlexIO2VGA vga4bit;
uint16_t fbWidth, fbHeight;

//=========================================
// Create a 4096 byte buffer for font file.
//=========================================
uint8_t font_buf[4096] = {0};
int32_t br = 0; // Bytes read counter.
int i = 0;

//=========================================
// Load a font from SD card oir USB drive.
int readFont(const char *filename) {
  // open the font file for reading. 
#if defined(USE_USB_DRV)
  myFile = firstPartition.open(filename);
#else
  myFile = SD.open(filename);
#endif
  br = myFile.read(font_buf, sizeof(font_buf));
  if(br <= 0) {
    vga4bit.printf("ERROR: %d read failed...\n",br);
    return br;
  }
  // close the file:
  myFile.close();
  return br;
}
//=========================================

void setup() {
  Serial.begin(9600);

  // Stop dislay if running to setup Display.
  vga4bit.stop();

  // Setup VGA display: 800x600x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits
  vga4bit.begin(*timing, false, false, 4);

  // Get display dimensions
  vga4bit.getFbSize(&fbWidth, &fbHeight);
  // Set fontsize 8x8 or (8x16 available)
  vga4bit.setFontSize(FONTSIZE,false);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLUE); 
  vga4bit.setForegroundColor(VGA_BRIGHT_BLUE);
  // Clear graphic screen
  vga4bit.clear(VGA_BRIGHT_BLUE);
  // Clear Print Window
  vga4bit.clearPrintWindow();
  // Move cursor to home position
  vga4bit.textxy(0,0);
 // Initialize text cursor:
  //  - block cursor 7 pixels wide and 8 pixels high
  //  - blink enabled
  //  - blink rate of 30
  vga4bit.initCursor(0,0,7,11,true,30);
  vga4bit.setTcursorColor(VGA_BRIGHT_WHITE);
  // Turn cursor on
  vga4bit.tCursorOn();
  
  vga4bit.setForegroundColor(VGA_BRIGHT_GREEN);
  vga4bit.printf("%cLOADABLE FONT DEMO  ");
  vga4bit.setForegroundColor(VGA_BRIGHT_RED);
  vga4bit.printf("(8x16 ONLY)   ");
  vga4bit.setForegroundColor(VGA_BRIGHT_YELLOW);

#if defined(USE_USB_DRV)
  vga4bit.printf("(USB DRIVE VERSION)\n\n");
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  // Start USBHost_t36, HUB(s) and USB devices.
  myusb.begin();
  myusb.Task();
  vga4bit.printf("\nInitializing USB MSC drive...");
  // Wait for claim proccess to finish.
  while(!firstPartition) {
    myusb.Task();
  }
#else
  vga4bit.printf("(SD CARD VERSION)\n\n");
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  vga4bit.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    vga4bit.println("initialization failed!");
    return;
  }
#endif

  vga4bit.println("initialization done.");
  
  // Load thin.fnt font file.
  vga4bit.printf("Press a key to load sample font: 'thin.fnt'\n\n");
  waitforInput();
  if(readFont((const char *)"thin.fnt") <= 0) {
    vga4bit.printf("\nStopping Here!!!\n");
    while(1);
  }
  // Write font to font memory buffer.
  vga4bit.fontLoadMem(font_buf);
  for(i = 0; i < 256; i++) if(i != 12) vga4bit.printf("%c",i); 

  // Load bold.fnt font file.
  vga4bit.printf("\n\nPress a key to load sample font: 'bold.fnt'\n\n");
  waitforInput();
  if(readFont((const char *)"bold.fnt") <= 0) {
    vga4bit.printf("\nStopping Here!!!\n");
    while(1);
  }
  // Write font to font memory buffer.
  vga4bit.fontLoadMem(font_buf);
  for(i = 0; i < 256; i++) if(i != 12) vga4bit.printf("%c",i); 

  // Load italics.fnt font file.
  vga4bit.printf("\n\nPress a key to load sample font: 'italics.fnt'\n\n");
  waitforInput();
  if(readFont((const char *)"italics.fnt") <= 0) {
    vga4bit.printf("\nStopping Here!!!\n");
    while(1);
  }
  // Write font to font memory buffer.
  vga4bit.fontLoadMem(font_buf);
  for(i = 0; i < 256; i++) if(i != 12) vga4bit.printf("%c",i); 

  // Load standard.fnt font file.
  vga4bit.printf("\n\nPress a key to load sample font: 'standard.fnt'\n\n");
  waitforInput();
  if(readFont((const char *)"standard.fnt") <= 0) {
    vga4bit.printf("\nStopping Here!!!\n");
    while(1);
  }
  // Write font to font memory buffer.
  vga4bit.fontLoadMem(font_buf);
  for(i = 0; i < 256; i++) if(i != 12) vga4bit.printf("%c",i); 

  vga4bit.printf("\n\nDONE...\n");
}

void loop() {
// Hang out here for a while...
}

void waitforInput()
{
  Serial.printf("Press any key to continue\n");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}
