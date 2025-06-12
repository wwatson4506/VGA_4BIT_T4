// VGA_T4_Box_Test.ino

#include "VGA_4bit_T4.h"
#include "box.h"

#define FONTSIZE 16

//const vga_timing *timing = &t1024x768x60;
//const vga_timing *timing = &t800x600x60;
//const vga_timing *timing = &t640x480x60;
const vga_timing *timing = &t640x400x70;

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

uint8_t *buf1, *buf2;
int i;
uint16_t r1, c1, r2, c2;
char c;

void setup() {
  while(!Serial);
  Serial.printf("%c",12);

  vga4bit.stop();
  // Setup VGA display: 1024x768x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits
  vga4bit.begin(*timing, false, false, 4);
  // Set fontsize 8x16 or (8x8 available)
  vga4bit.setFontSize(FONTSIZE, false);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLACK); 
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  // Clear screen to background color
  vga4bit.clear(VGA_BLACK);

  box_charfill(1, 1, 23, vga4bit.getTwidth()-1, 176);
  buf1 = vbox_get(5, 5, 15, 40);
  buf2 = vbox_get(1, 1, 22, vga4bit.getTwidth()-1);

  vga4bit.setForegroundColor(VGA_BRIGHT_GREEN);
  vga4bit.setBackgroundColor(VGA_BLUE);

  box_erase(5, 5, 15, 40);
  box_draw(7, 5, 7, 40, 1);
  box_draw(5, 5, 15, 40, 2);

  vga4bit.textxy(9, 11);
  vga4bit.printf("Press any key to continue...");

  waitforInput();
  vbox_put(buf1);
  free(buf1);

  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  vga4bit.setBackgroundColor(VGA_BLACK);

  vga4bit.textxy(1, 24);
  vga4bit.printf("Press any key to continue...");
  waitforInput();
  for(i = 0; i < 100; i++) {
    do {
      r1 = rand() % 19 + 3;
      r2 = rand() % 19 + 3;
    }while( r2 < r1);
    do {
      c1 = rand() % 70 + 5;
      c2 = rand() % 70 + 5;
    }while( c2 < c1);
    vga4bit.setForegroundColor(rand() % 16);
    vga4bit.setBackgroundColor(rand() % 16);
    box_erase(r1, c1, r2, c2);
    box_draw(r1, c1, r2, c2, rand() % 2 + 1);
  }
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  vga4bit.setBackgroundColor(VGA_BLACK);
  vga4bit.textxy(1, 24);
  vga4bit.printf("Press any key to continue...");
  waitforInput();
  for(i=0; i < 100; i++) {
    do {
      r1 = rand() % 20 + 3;
      r2 = rand() % 20 + 3;
    }while (r2 < r1);
    do {
      c1 = rand() % 70 + 5;
      c2 = rand() % 70 + 5;
    }while(c2 < c1);
      vga4bit.setForegroundColor(rand() % 16);
      vga4bit.setBackgroundColor(rand() % 16);
      box_erase(r1,c1,r2,c2);
      box_charfill(r1,c1,r2,c2, (char)(rand() % 26 + 65));
  }
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  vga4bit.setBackgroundColor(VGA_BLACK);
  vga4bit.textxy(1, 24);
  vga4bit.printf("Press any key to continue...");
  waitforInput();
  vbox_put(buf2);
  free(buf2);
  box_erase(24,1,24,vga4bit.getTwidth()-1);
  vga4bit.textxy(1, 24);
  vga4bit.printf("DONE...");
}

void loop() {
  yield();
}

void waitforInput()
{
  Serial.printf("Press anykey to continue\n");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}
