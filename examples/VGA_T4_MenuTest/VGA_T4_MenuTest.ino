// Testing.ino 640x480_Screen_8x8_font_4_windows.ino

#include "USBHost_t36.h"
#include "VGA_4bit_T4.h"
#include "VGA_T4_Config.h"
#include "box.h"
#include "menu.h"
#include "mtext.h"

// Setup USB Host
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);

USBHIDParser hid1(myusb); // Needed for USB mouse.
USBHIDParser hid2(myusb); // Needed for USB wireless keyboard/mouse combo.

// Create an instance of vga4bit.
FlexIO2VGA vga4bit;

// Uncomment one of the following screen resolutions. Try them all:)
//const vga_timing *timing = &t1024x768x60; // Not enough memory avilable for this screen size in this sketch.
//const vga_timing *timing = &t800x600x60;
//const vga_timing *timing = &t640x480x60;
const vga_timing *timing = &t640x400x70;

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

#define FONTSIZE 16

void setup() {
  // Wait for USB Serial
  while (!Serial && (millis() < 5000)) {
    yield();
  }	

  Serial.printf("%c\n",12);
  if(CrashReport) Serial.println(CrashReport);
  
  // Startup USBHost
  myusb.begin();
  delay(500);
  myusb.Task();
  USBKeyboardInit();

  vga4bit.stop();
  // Setup VGA display: 800x600x60
  //                    double Width  = false
  //                    double Height = false
  //                    Color Depth   = 4 bits, Mono not supported yet.
  vga4bit.begin(*timing, false, false, 4);
  // Set fontsize 8x8 or (8x16 available)
  vga4bit.setFontSize(FONTSIZE, false);
  // Set default foreground and background colors
  vga4bit.setBackgroundColor(VGA_BLACK); 
  vga4bit.setForegroundColor(VGA_BRIGHT_WHITE);
  // Clear screen to background color
  vga4bit.clear(VGA_BLUE);
  // Initialize text cursor:
  //  - block cursor 8 pixels wide and 8 pixels high
  //  - blink enabled
  //  - blink rate of 30
  vga4bit.initCursor(0,14,7,15,true,30);
  // Set white text cursor
  vga4bit.setTcursorColor(VGA_BRIGHT_WHITE);
  // Move cursor to home position
  vga4bit.textxy(0,0);
  vga4bit.tCursorOn();

}

void loop() {
  int	n, n1;
  int choice = 1; // Default to first menu item.
  int	quit_flag = 0;
  int	first_time = 1;
  uint8_t *save_bar_main;
  uint8_t *save_info_box;
  uint8_t *save_drop_colors;
  
  uint8_t fColor = vga4bit.getTextFGC();  
  uint8_t bColor = vga4bit.getTextBGC();  
  uint8_t curx = vga4bit.getTextX();
  uint8_t cury = vga4bit.getTextY();

  choice = 1;

  vga4bit.setForegroundColor(VGA_WHITE);
  vga4bit.clearPrintWindow();
  vga4bit.tCursorOff();	// turn cursor off	
    
    // Fill screen with background pattern.
    box_charfill(0, 0, vga4bit.getTheight()-1, vga4bit.getTwidth()-1, B_PATTERN);

	while(!quit_flag) {
		// Erase the info box and main menu bar if not first time
		if(!first_time) {
			menu_erase(save_info_box);
			menu_erase(save_bar_main);
		} else {
			first_time = 0; // Flag not first time.
        }

		// Display the information box
		save_info_box = menu_message(10, 8, info_box);

		// The main menu bar
	  save_bar_main = menu_bar(3, 18, bar_main, &choice);

		switch(choice) {
			case	1: // Main menu bar lines entry.
				menu_erase(menu_drop(4, 18, drop_lines, &n));
				if(n)
					menu_box_lines(n); // Draw border lines. (single/double)
				break;
			case	2: // Main menu bar colors entry.
				save_drop_colors = menu_drop(4, 25, drop_colors, &n);
				 // Colors sub menu entrys (1 == background, 8 == Hilited background)
				if(n > 1 && n < 8) {
					// drop_t_colors, see mtext.h
					menu_erase(menu_drop(6, 50, drop_t_colors, &n1));
					n1--;
					switch(n) { // Colors sub menu entrys. (2 - 7)
						case	2:
							menu_line_color(n1);
							break;
						case	3:
							menu_title(n1);
							break;
						case	4:
							menu_text_color(n1);
							break;
						case	5:
							menu_prompt(n1);
							break;
						case	6:
							menu_hiletter(n1);
							break;
						case	7:
							menu_hitext(n1);
							break;
					}
				} else if(n == 1 || n == 8) { // do sub menu entrys 1 and 8.
					menu_erase(menu_drop(6, 50, drop_t_colors, &n1));
					n1--;
					switch(n) {
						case 1:
							menu_back((int)n1);
							break;
						case 8:
							menu_hilight_back((int)n1);
							break;
					}
				}
				menu_erase(save_drop_colors); // Close box and free buffer.
				break;
			case	3: // Main menu bar shadow entry.
				menu_erase(menu_drop( 4, 33, drop_shadow, &n));
				if(n)
					menu_shadow(--n);
				break;
			case	4: // Main menu bar quit entry.
				menu_erase(menu_drop(4, 41, drop_quit, &n));
				if(n == 2)
					quit_flag = 1; // Quit while loop.
				break;
			default:
				// we'll ignore the escape key at this level
				break;
		}
	} 
    vga4bit.setForegroundColor(fColor);
    vga4bit.setBackgroundColor(bColor);
    vga4bit.clearPrintWindow();
    vga4bit.textxy(curx,cury);
    vga4bit.tCursorOn(); // turn cursor on	
    vga4bit.printf("Press any key to run again...\n"); // Run again?
    USBGetkey();
}

void waitforInput() {
  Serial.println("Press anykey to continue");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}

