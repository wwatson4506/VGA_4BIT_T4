// Graphics Drawing Test
// Taken from uVga and modified for T4.1 4 bit VGA driver by jmarsh.
// Polygon drawing taken from VGA_T4 by Jean-MarcHarvengt
// at https://github.com/Jean-MarcHarvengt/VGA_t4 and modified
// for VGA_4bit_T4.
// *********************************************************
// drawEllipse() and fillEllipse() are not working properly.
// Try fillEllips(516, 419,59,61,VGA_BRIGHT_WHITE).
// Not displayed properly. Have not figured out why yet.
// *********************************************************

#include "VGA_4bit_T4.h"

// Uncomment one of the following screen resolutions. Try them all:)
//const vga_timing *timing = &t1024x768x60;
//const vga_timing *timing = &t800x600x60;
const vga_timing *timing = &t640x480x60;
//const vga_timing *timing = &t640x400x70;
FlexIO2VGA vga4bit;

// Wait for frame update complete. If set to false, updates are very
// fast but causes flicker.
bool wait = true; 
// This is the number of interations per function. If wait is set to
// false, itr should be set to at least 10000.
int itr = 500;

uint16_t fbWidth, fbHeight;

void setup() {
  Serial.begin(9600);

  vga4bit.stop();
  // Setup VGA display: 800x600x60
  //                    double Height = false
  //                    double Width  = false
  //                    Color Depth   = 4 bits
  vga4bit.begin(*timing, false, false, 4);
  // Get display dimensions
  vga4bit.getFbSize(&fbWidth, &fbHeight);
  vga4bit.wait_for_frame();
  // Clear graphic screen
  vga4bit.clear(VGA_BLACK);

  // Polygon data
  PolySet.Center.x = 40     ; PolySet.Center.y = 40 ; 
  PolySet.Pts[0].x = 30     ; PolySet.Pts[0].y = 10 ;
  PolySet.Pts[1].x = 40     ; PolySet.Pts[1].y = 20 ;
  PolySet.Pts[2].x = 40     ; PolySet.Pts[2].y = 40 ;
  PolySet.Pts[3].x = 60     ; PolySet.Pts[3].y = 40 ;
  PolySet.Pts[4].x = 60     ; PolySet.Pts[4].y = 70 ;
  PolySet.Pts[5].x = 50     ; PolySet.Pts[5].y = 70 ;
  PolySet.Pts[6].x = 50     ; PolySet.Pts[6].y = 50 ;
  PolySet.Pts[7].x = 20     ; PolySet.Pts[7].y = 50 ;
  PolySet.Pts[8].x = 20     ; PolySet.Pts[8].y = 40 ;
  PolySet.Pts[9].x = 10     ; PolySet.Pts[9].y = 40 ;
  PolySet.Pts[10].x = 10    ; PolySet.Pts[10].y = 30 ;
  PolySet.Pts[11].x = 20    ; PolySet.Pts[11].y = 30 ;
  PolySet.Pts[12].x = 10000 ; // ****** last point.x >= 10000 ******
}

void loop() {
  int i, r;
  static vga_text_direction dir[] = { VGA_DIR_RIGHT, VGA_DIR_TOP, VGA_DIR_LEFT, VGA_DIR_BOTTOM };
  int16_t x0, y0, x1, y1, y2;


  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth);
    y0=random(fbHeight);
    x1=random(fbWidth);
    y1=random(fbHeight);
    y2=random(fbHeight);
    vga4bit.drawTriangle(x0,y0,x1,y1,y2,y2,random(16));
  }

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth);
    y0=random(fbHeight);
    x1=random(fbWidth);
    y1=random(fbHeight);
    y2=random(fbHeight);
    vga4bit.fillTriangle(x0,y0,x1,y1,y2,y2,random(16));
  }
  
  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth);
    y0=random(fbHeight);
    x1=random(fbWidth);
    y1=random(fbHeight);
    vga4bit.drawRect(x0,y0,x1,y1,random(16));
  }

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth);
    y0=random(fbHeight);
    x1=random(fbWidth);
    y1=random(fbHeight);
    vga4bit.fillRect(x0,y0,x1,y1,random(16));
  }

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth);
    y0=random(fbHeight);
    x1=random(fbWidth);
    y1=random(fbHeight);
    r=random(50)+5;
    vga4bit.drawRrect(x0,y0,x1,y1,r,random(16));
  }

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth);
    y0=random(fbHeight);
    x1=random(fbWidth);
    y1=random(fbHeight);
    r=random(50)+5;
    vga4bit.fillRrect(x0,y0,x1,y1,r,random(16));
  }

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth-1);
    y0=random(fbHeight-1);
    x1=random(100)+20;
    y1=random(100)+20;
    vga4bit.drawEllipse(x0,y0,x1,y1,random(16));
  }	

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth-1);
    y0=random(fbHeight-1);
    x1=random(100)+20;
    y1=random(100)+20;
    vga4bit.fillEllipse(x0, y0, x1, y1, random(16));
  }	

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth);
    y0=random(fbHeight);
    r=random(50);
    vga4bit.drawCircle(x0,y0,r,1,random(16));
  }	

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth);
    y0=random(fbHeight);
    r=random(50);
    vga4bit.fillCircle(x0,y0,r,random(16));
  }	

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth);
    y0=random(fbHeight);
    r=random(5)-1;
    vga4bit.drawText(x0,y0,"Hello Teensy!",random(16),random(16),dir[r]);
  }
  
  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for(i = 0; i<=itr; i++) {
    vga4bit.fbUpdate(wait); //wait_for_frame();
    x0=random(fbWidth);
    y0=random(fbHeight);
    r=random(5)-1;
    vga4bit.drawText(x0,y0,"VGA_4_BIT Library",random(16),random(16),dir[r]);
  }

  delay(2000);

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for (int i = 0;i<100;i++){
    vga4bit.fbUpdate(wait);
    vga4bit.drawpolygon(random(fbWidth) , random(fbHeight), random(1,15));
  }
  delay(2000);
  vga4bit.clear(VGA_BLACK);
  for (int i = 0;i<100;i++){
    vga4bit.fbUpdate(wait);
    vga4bit.drawfullpolygon(random(fbWidth) , random(fbHeight), random(1,15),random(1,15));
  }
  delay(2000);
  vga4bit.clear(VGA_BLACK);
  for (int i = 0;i<100;i++){
    vga4bit.fbUpdate(wait);
    vga4bit.drawrotatepolygon(random(fbWidth) , random(fbHeight) , random(360), random(1,15), random(1,15), 0);
  }
  delay(2000);
  vga4bit.clear(VGA_BLACK);
  for (int i = 0;i<100;i++){
   vga4bit.fbUpdate(wait);
   vga4bit.drawrotatepolygon(random(fbWidth) , random(fbHeight) , random(360), random(1,15), random(1,15), 1);
  }
  delay(2000);
  vga4bit.clear(VGA_BLACK);
  for (int i = 0;i<1000;i++){
    vga4bit.drawrotatepolygon(200 , 240 , i % 360, random(1,15), random(1,15), 0);
    vga4bit.drawrotatepolygon(400 , 240 , i % 360, random(1,15), random(1,15), 1);
    vga4bit.fbUpdate(wait);
    delay(13);
    vga4bit.clear(VGA_BLACK);
  }
  delay(2000);
  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for (int i = 0;i<1000;i++){
    vga4bit.fillQuad(random(fbWidth) , random(fbHeight) ,random(150)+10 , random(150)+10 ,random(360) , random(16));
    vga4bit.fbUpdate(wait);
  }
  delay(4000);

  vga4bit.clear(VGA_BLACK); // Clear graphic screen
  for (int i = 0;i<100;i++){
    vga4bit.drawQuad(random(fbWidth) , random(fbHeight) ,random(150)+10 , random(150)+10,random(360) , random(16));
    vga4bit.fbUpdate(wait);
  }
  delay(4000);

  vga4bit.clear(VGA_BLACK);
}
