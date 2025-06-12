#include "USBHost_t36.h"
#include "SD.h"
#include "VGA_4bit_T4.h"
#include "vt100.h"
#include "USBKeyboard.h"
#include "kilo.h"

// Uncomment to enable USB drive usage.
//#define USE_USB_DRIVE 1

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
//USBHub hub3(myusb);
//USBHub hub4(myusb);

#ifdef USE_USB_DRIVE
// Setup MSC for the number of USB Drives you are using. (one for this
// example). Mutiple  USB drives can be used. Hot plugging is supported.
USBDrive myDrive(myusb);
USBFilesystem fs1(myusb);

// Volume name buffer
char	volumeName[32];
elapsedMillis mscTimeOut; 
uint8_t errCode = MS_CBW_PASS;
#endif

// Uncomment one of the following screen resolutions. Try them all:)
//const vga_timing *timing = &t1024x768x60; // Not enough memory avilable for this screen size in this sketch.
const vga_timing *timing = &t800x600x60;
//const vga_timing *timing = &t640x480x60;
//const vga_timing *timing = &t640x400x70;

// Must use this instance name. It's used in the driver.
FlexIO2VGA vga4bit;
static int fb_width, fb_height;

#define FONTSIZE 16

char sbuff[512];

void setup() {
  Serial.begin(9600);

  // Startup USBHost
  myusb.begin();
  // Init VT100

  vga4bit.stop();
  // Setup VGA display: 800x600x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits, Mono not supported yet.
  vga4bit.begin(*timing, false, false, 4);
  // Get display dimensions
  vga4bit.getFbSize(&fb_width, &fb_height);
  // Set fontsize 8x8 or (8x16 available)
  vga4bit.setFontSize(FONTSIZE, false);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLUE); 

  fb_width = vga4bit.getTwidth();
  fb_height = vga4bit.getTheight();
  
  //=====================================================
  // Clear the character print window in 800x600 mode
  // with a font height of 16 needs fb_height adjusted
  // to 38 characters as 600 / (font_height == 16) = 37.5
  // character lines. which returns an int of 37. So we
  // set fb_height to 38 so complete screen is cleared.
  // A font_height of 8 gives an even 75 character lines.
  // The following two lines set the proper
  //======================================================
  if((fb_height == 37) && (FONTSIZE == 16)) fb_height = 38; // Adjust if needed.
  vga4bit.setPrintCWindow(0, 0, fb_width, fb_height); // Set print window size.

  // Now clear Print Window
  vga4bit.clearPrintWindow();
  // Initialize text cursor:
  //  - block cursor 8 pixels wide and 16 pixels high
  //  - blink enabled
  //  - blink rate of 30
  vga4bit.initCursor(0,0,7,FONTSIZE - 1,true,30);
  // Turn cursor on
  vga4bit.tCursorOn();
  // Move cursor to home position
  vga4bit.textxy(0,0);

  initVT100();
  // Init USB Keyboard
  USBKeyboardInit();

  vga4bit.setForegroundColor(VGA_BRIGHT_GREEN);
  vga4bit.printf("KILO EDITOR T4.1 VERSION\n");
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);

#ifdef USE_USB_DRIVE
  // There is a slight delay after a USB MSC device is plugged in which
  // varys with the device being used. The device is internally 
  // initializing.
  vga4bit.print("\nInitializing USB MSC drive...");
  mscTimeOut = 0;
  while(!myDrive) {
    if((mscTimeOut > MSC_CONNECT_TIMEOUT) && (myDrive.errorCode() == MS_NO_MEDIA_ERR)) {
      vga4bit.printf("\nNo drive connected yet!!!!");
      vga4bit.printf("\nConnect a drive to continue...\n");
      while(!myDrive);
    } 
	delay(1);
  }
  mscTimeOut = 0;
  // Wait for claim proccess to finish.
  // But not forever.
  while(!fs1) {
    myusb.Task();
    if(mscTimeOut >  MSC_CONNECT_TIMEOUT) {
      vga4bit.print("Timeout --> error code: ");
      vga4bit.println(myDrive.errorCode(), DEC);
      vga4bit.println("Halting...");
      while(1);
    }
    delay(1);
  }
  vga4bit.printf("initialization done.");
#endif

  // Init SD card if available
  vga4bit.print("\nInitializing SD drive...");
  if (!SD.begin(BUILTIN_SDCARD)) vga4bit.printf("\nSD Card: Initialize Error.\n");
  vga4bit.printf("initialization done.\n");
 
}

void loop() {
  int etype = 0;
  uint8_t c = 0;

#ifdef USE_USB_DRIVE
  
  myusb.Task();

  vga4bit.println("\nUSB drive Directory Listing:");
  // Get the volume name if it exists.
  fs1.mscfs.getVolumeLabel(volumeName, sizeof(volumeName));
  vga4bit.print("Volume Name: ");
  vga4bit.println(volumeName);
  File root = fs1.open("/");
  printDirectory(root, 0);

  vga4bit.printf("\nPress any key to show SD directory...\n");
  USBGetkey();
#endif
  vga4bit.printf("\nSD directory listing:\n");
  File root1 = SD.open("/");
  printDirectory(root1, 0);

  vga4bit.printf("\nEnter filename to edit: ");
  cgets(sbuff);

#ifdef USE_USB_DRIVE
  vga4bit.printf("\nEnter drive to use 1 = SD drive\n");
  vga4bit.printf("                   2 = USB drive...\n");
  c = USBGetkey();
  switch(c) {
    case '1':
#endif
      if(SD.exists(sbuff)) {
		etype = kilo(&SD, sbuff);
        if(etype == -2) {
		  vga4bit.printf("File is to large to edit...\n");
          vga4bit.printf("Press any key to continue...\n");
          USBGetkey();
#ifdef USE_USB_DRIVE
		  break;
#endif
		}
	  } else {
		vga4bit.printf("File not found!\n");
        vga4bit.printf("Create %s ? (y/n)\n",sbuff);
        c = USBGetkey();
        if(c == 'y' || c == 'Y') etype = kilo(&SD, sbuff);
		vga4bit.printf("Press any key to continue...\n");
        USBGetkey();
	  }

#ifdef USE_USB_DRIVE
      break;
    case '2':
      if(fs1.exists(sbuff)) {
		etype = kilo(&fs1, sbuff);
        if(etype == -2) {
		  vga4bit.printf("File is to large to edit...\n");
          vga4bit.printf("Press any key to continue...\n");
          USBGetkey();
		  break;
		}
	  } else {
		vga4bit.printf("File not found!\n");
        vga4bit.printf("Create %s ? (y/n)\n",sbuff);
        c = USBGetkey();
        if(c == 'y' || c == 'Y') etype = kilo(&fs1, sbuff);
		vga4bit.printf("Press any key to continue...\n");
        USBGetkey();
	  }
      break;
  }
#endif
  // Reset text cursor
  vga4bit.initCursor(0,0,7,15,true,30);
  vga4bit.tCursorOn();
//  vga4bit.clear(VGA_BLUE);
  vga4bit.clearPrintWindow();
}


void printDirectory(File dir, int numSpaces) {
   while(true) {
     File entry = dir.openNextFile();
     if (! entry) {
       break;
     }
     printSpaces(numSpaces);
     vga4bit.print(entry.name());
     if (entry.isDirectory()) {
       vga4bit.println("/");
       printDirectory(entry, numSpaces+2);
     } else {
       // files have sizes, directories do not
       printSpaces(35 - numSpaces - strlen(entry.name()));
       vga4bit.print("  ");
       vga4bit.printf(F("%10ld"),entry.size());
       DateTimeFields datetime;
       if (entry.getModifyTime(datetime)) {
         printSpaces(4);
         printTime(datetime);
       }
       vga4bit.println();
     }
     entry.close();
   }
}

void printSpaces(int num) {
  for (int i=0; i < num; i++) {
    vga4bit.print(" ");
  }
}

void printTime(const DateTimeFields tm) {
  const char *months[12] = {
    "January","February","March","April","May","June",
    "July","August","September","October","November","December"
  };
  if (tm.hour < 10) vga4bit.print('0');
  vga4bit.print(tm.hour);
  vga4bit.print(':');
  if (tm.min < 10) vga4bit.print('0');
  vga4bit.print(tm.min);
  vga4bit.print("  ");
  vga4bit.print(months[tm.mon]);
  vga4bit.print(" ");
  vga4bit.print(tm.mday);
  vga4bit.print(", ");
  vga4bit.print(tm.year + 1900);
}
