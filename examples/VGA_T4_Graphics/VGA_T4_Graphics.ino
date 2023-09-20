// Graphics Drawing Test
// Taken from uVga and modified for T4.1 4 bit VGA driver by jmarsh.

// *********************************************************
// drawEllipse() and fillEllipse() are not working properly.
// Try fillEllips(516, 419,59,61,VGA_BRIGHT_WHITE).
// Not displayed properly. Have not figured out why yet.
// *********************************************************

#include "VGA_4bit_T4.h"

const vga_timing *timing = &t640x480x60;
FlexIO2VGA vga4bit;

// Wait for frame update complete. If set to false, updates are very
// fast but causes flicker.
bool wait = true; 
// This is the number of interations per function. If wait is set to
// false, itr should be set to at least 10000.
int itr = 500;

int fb_width, fb_height;

void setup() {
  Serial.begin(9600);

  vga4bit.stop();
  // Setup VGA display: 800x600x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits
  vga4bit.begin(*timing, false, false, 4);
  // Get display dimensions
  vga4bit.getFbSize(&fb_width, &fb_height);
  vga4bit.wait_for_frame();
  // Clear graphic screen
  vga4bit.clear(VGA_BLACK);
}

void loop() {
  int i, r;
  static vga_text_direction dir[] = { VGA_DIR_RIGHT, VGA_DIR_TOP, VGA_DIR_LEFT, VGA_DIR_BOTTOM };
  int16_t x0, y0, x1, y1, y2;


  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fb_width);
    y0=random(fb_height);
    x1=random(fb_width);
    y1=random(fb_height);
    y2=random(fb_height);
    vga4bit.drawTriangle(x0,y0,x1,y1,y2,y2,random(16));
  }

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fb_width);
    y0=random(fb_height);
    x1=random(fb_width);
    y1=random(fb_height);
    y2=random(fb_height);
    vga4bit.fillTriangle(x0,y0,x1,y1,y2,y2,random(16));
  }
  
  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fb_width);
    y0=random(fb_height);
    x1=random(fb_width);
    y1=random(fb_height);
    vga4bit.drawRect(x0,y0,x1,y1,random(16));
  }

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fb_width);
    y0=random(fb_height);
    x1=random(fb_width);
    y1=random(fb_height);
    vga4bit.fillRect(x0,y0,x1,y1,random(16));
  }

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fb_width-1);
    y0=random(fb_height-1);
    x1=random((fb_width-1)/2);
    y1=random((fb_height-1)/2);
    vga4bit.drawEllipse(x0,y0,x1,y1,random(16));
  }	

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fb_width-1);
    y0=random(fb_height-1);
    x1=random((fb_width-1)/2);
    y1=random((fb_height-1)/2);
    vga4bit.fillEllipse(x0, y0, x1, y1, random(16));
  }	

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fb_width);
    y0=random(fb_height);
    r=random(100);
    vga4bit.drawCircle(x0,y0,r,random(16));
  }	

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fb_width);
    y0=random(fb_height);
    r=random(100);
    vga4bit.fillCircle(x0,y0,r,random(16));
  }	

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fb_width);
    y0=random(fb_height);
    r=random(5)-1;
    vga4bit.drawText(x0,y0,"Hello Teensy!",random(16),random(16),dir[r]);
  }
  
  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fb_width);
    y0=random(fb_height);
    r=random(5)-1;
    vga4bit.drawText(x0,y0,"VGA_4_BIT Library",random(16),random(16),dir[r]);
  }
}
