/*
 VGA_T4_treedee.ino
 Taken from Sumotoy's RA8875 library and adapted for
 4 bit VGA driver on Teensy 4.1.
*/

#include "VGA_4bit_T4.h"
#include <math.h>

#define FONTSIZE 16

//const vga_timing *timing = &t1024x768x60;
//const vga_timing *timing = &t800x600x60;
const vga_timing *timing = &t640x480x60;
//const vga_timing *timing = &t640x400x70;

// Must use this instance name (vga4bit). It is used in the driver.
FlexIO2VGA vga4bit;
//static int fb_width, fb_height;

// Array of vga4bit Basic Colors
const uint8_t myColors[] = {
  VGA_BLACK,  // 0
  VGA_BLUE,   // 1
  VGA_GREEN,  // 2
  VGA_CYAN,   // 3
  VGA_RED,    // 4
  VGA_MAGENTA,// 5
  VGA_YELLOW, // 6
  VGA_WHITE,  // 7
  VGA_GREY,   // 8
  VGA_BRIGHT_BLUE,  // 9
  VGA_BRIGHT_GREEN, // 10
  VGA_BRIGHT_CYAN,  // 11
  VGA_BRIGHT_RED,   // 12
  VGA_BRIGHT_MAGENTA, // 13
  VGA_BRIGHT_YELLOW,  // 14
  VGA_BRIGHT_WHITE  // 15
};

// A small hex dump function
void hexDump(const void *ptr, uint32_t len) {
  uint32_t  i = 0, j = 0;
  uint8_t   c=0;
  const uint8_t *p = (const uint8_t *)ptr;

  Serial.printf("BYTE      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
  Serial.printf("---------------------------------------------------------\n");
  for(i = 0; i <= (len-1); i+=16) {
   Serial.printf("%4.4lx      ",i);
   for(j = 0; j < 16; j++) {
      c = p[i+j];
      Serial.printf("%2.2x ",c);
    }
    Serial.printf("  ");
    for(j = 0; j < 16; j++) {
      c = p[i+j];
      if(c > 31 && c < 127)
        Serial.printf("%c",c);
      else
        Serial.printf(".");
    }
    Serial.printf("\n");
  }
}

void setup() {
  Serial.begin(9600);
//  Serial.print(CrashReport);
  vga4bit.stop();
  // Setup VGA display: 1024x768x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits
  vga4bit.begin(*timing, false, false, 4);
  // Get display dimensions
//  vga4bit.getFbSize(&fb_width, &fb_height);
  // Set fontsize 8x16 or (8x8 available)
  vga4bit.setFontSize(FONTSIZE);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLUE); 
  vga4bit.setForegroundColor(VGA_BRIGHT_GREEN);
  // Clear screen to background color
  vga4bit.clear(VGA_BLUE);

  vga4bit.initGcursor(FILLED_ARROW,0,0,vga4bit.getFontWidth()-1,vga4bit.getFontHeight()-1);
  vga4bit.gCursorOn();
  vga4bit.setCursorType(I_BEAM);
  
  vga4bit.initCursor(0,0,1,15,true,30); // Define I-beam text cursor.
  vga4bit.cursorOn();
  
  vga4bit.printf("This is a test line.\n"); 
  vga4bit.textxy(2,0);
  vga4bit.drawText(300,200,"Hello Teensy!",myColors[15],myColors[1],VGA_DIR_RIGHT);

}

void loop() {
	int fb_width, fb_height;

	vga4bit.getFbSize(&fb_width, &fb_height);

	waitforInput();
	vga4bit.scroll(0,0,fb_width,fb_height,0,16, myColors[1]); // Scroll down
	waitforInput();
	vga4bit.scroll(0,0,fb_width,fb_height,0,-16, myColors[1]); // Scroll up
}

void printDirectory(File dir, int numSpaces) {
   while(true) {
     File entry = dir.openNextFile();
     if (! entry) {
       //Serial.println("** no more files **");
       break;
     }
     printSpaces(numSpaces);
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numSpaces+2);
     } else {
       // files have sizes, directories do not
       printSpaces(48 - numSpaces - strlen(entry.name()));
       Serial.print("  ");
       Serial.print(entry.size(), DEC);
       DateTimeFields datetime;
       if (entry.getModifyTime(datetime)) {
         printSpaces(4);
         printTime(datetime);
       }
       Serial.println();
     }
     entry.close();
   }
}

void printSpaces(int num) {
  for (int i=0; i < num; i++) {
    Serial.print(" ");
  }
}

void printTime(const DateTimeFields tm) {
  const char *months[12] = {
    "January","February","March","April","May","June",
    "July","August","September","October","November","December"
  };
  if (tm.hour < 10) Serial.print('0');
  Serial.print(tm.hour);
  Serial.print(':');
  if (tm.min < 10) Serial.print('0');
  Serial.print(tm.min);
  Serial.print("  ");
  Serial.print(months[tm.mon]);
  Serial.print(" ");
  Serial.print(tm.mday);
  Serial.print(", ");
  Serial.print(tm.year + 1900);
}

void waitforInput()
{
  Serial.println("Press anykey to continue");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}
