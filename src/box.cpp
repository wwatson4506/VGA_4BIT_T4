// Box.cpp
//
// A simple character based box and border drawing library.

#include <Arduino.h>
#include "box.h"
#include "VGA_4bit_T4.h"

// Save a rectanglar section of frame buffer memory to allocated memory.
// The first 4 16bit locations hold  the coordinates (for vbox_put().
uint8_t *vbox_get(int16_t row1, int16_t col1, int16_t row2, int16_t col2) {
  int16_t width, height, tWidth, tHeight;
  uint16_t bytes;
  uint8_t *buf, *bufptr;

  // Keep in bounds of current character display.
  tHeight = vga4bit.getTheight();
  if(row2 >= tHeight) row2 = tHeight-1;
  tWidth = vga4bit.getTwidth();
  if(col2 >= tWidth) row2 = tWidth-1;
  
  // calculate dimensions in bytes
  width  = (col2 - col1) + 1;
  height = (row2 - row1) + 1;
  bytes  = (height * width) + 8;

  // allocate storage space
  if((buf = (uint8_t *)malloc(sizeof(uint8_t)*bytes)) == NULL) {
    puts("vbox_get(): malloc() Failed\n");
    while(1);
  }
  
  //======================================================
  // Save the box coordinates in the buffer.
  // First 8 bytes used to store four 16 bit coorrdinates.
  //======================================================
  bufptr = buf;
  *bufptr++ = (uint8_t)((row1 >> 8) & 0xff);
  *bufptr++ = (uint8_t)(row1 & 0xff);
  *bufptr++ = (uint8_t)((col1 >> 8) & 0xff);
  *bufptr++ = (uint8_t)(col1 & 0xff);
  *bufptr++ = (uint8_t)((row2 >> 8) & 0xff);
  *bufptr++ = (uint8_t)(row2 &0xff);
  *bufptr++ = (uint8_t)((col2 >> 8) & 0xff);
  *bufptr++ = (uint8_t)(col2 & 0xff);

  // grab each line of video
  for(uint16_t i = row1; i < row2; i++) {
    for(uint16_t j = col1; j < col2+1; j++) {
      vga4bit.getChar(j, i,bufptr);
    }
  }
  return(buf);
}

//======================================================
// Restore a section of frame buffer memory from saved memory.
// Calling function must free malloced buffer after use.
//======================================================
void vbox_put(uint8_t *buf) {
  int16_t row1, col1, row2, col2;
  uint8_t *workbuf;

  // get the coordinates back
  workbuf = buf;

  // Convert 8 bit pairs to 16 bit coords (Big Endian).
  row1 = (uint16_t)((workbuf[0] << 8) + (workbuf[1] & 0xff));
  col1 = (uint16_t)((workbuf[2] << 8) + (workbuf[3] & 0xff));
  row2 = (uint16_t)((workbuf[4] << 8) + (workbuf[5] & 0xff));
  col2 = (uint16_t)((workbuf[6] << 8) + (workbuf[7] & 0xff));
  workbuf+=8; // Skip over coords to data.

  // Write out each line of video
  for(uint16_t i = row1; i < row2; i++) {
    for(uint16_t j = col1; j < col2+1; j++) {
      vga4bit.putChar(j, i, workbuf);
    }
  }
}

//=======================================================
// Fill a a rectangular section of frame buffer to with a
// ascii character.
//=======================================================
void box_charfill(int16_t row1, int16_t col1, int16_t row2, int16_t col2, char c) {
  int16_t x, y;
  for(y = row1; y <= row2; y++) {
    for(x = col1; x <= col2; x++) {
      vga4bit.textxy(x,y);
      vga4bit.write(c);
    }
  }
}

//======================================================
// Clears a rectangular section of frame buffer to the
// current text foreground color. Use a ascii block
// character 219 with fillColor.
//======================================================
void box_color(uint8_t row1, uint8_t col1, uint8_t row2, uint8_t col2, uint8_t fillColor) {
  uint8_t tempColor = vga4bit.getFGC();
  vga4bit.setForegroundColor(fillColor);
  uint8_t x, y;
  for(y = row1; y <= row2; y++) {
    for(x = col1; x <= col2; x++) {
      vga4bit.textxy(x,y);
      vga4bit.write(219);
    }
  }
  vga4bit.setBackgroundColor(tempColor);
}

//============================================================
// Clears a rectangular section of character memory to spaces.
//============================================================
void box_erase(int16_t row1, int16_t col1, int16_t row2, int16_t col2) {
  int16_t x, y;
  for(y = row1; y <= row2; y++) {
    for(x = col1; x <= col2; x++) {
      vga4bit.textxy(x,y);
      vga4bit.write(' ');
    }
  }
}

//=======================================================
// Box_Draw routine. Draws a single or double line around
// as a border to a box.
//=======================================================
void box_draw(int16_t row1, int16_t col1, int16_t row2, int16_t col2, int line_type) {
  int16_t x, y, dx, dy;
  unsigned char c;

  // Work around the box
  x = col1;
  y = row1;
  dx = 1;
  dy = 0;	
  do {
    // Set the default character for the unbordered boxes.
    c = ' ';
    // Set the single-line drawing character
    if(line_type == 1) {
      if(dx != 0) {
        c = 196;
      } else {
        c = 179;
      }
      // Set the double-line drawing character
      } else if(line_type == 2) {
        if(dx != 0) {
          c = 205;
        } else {
          c = 186;
        }
      }
      // Change directions at top right corner
      if((dx == 1) && (x == col2)) {
        dx = 0;
        dy = 1;
        if(line_type == 1) {
          c = 191;
        } else if(line_type == 2) {
          c = 187;
        }
      }
      // Change direction at bottom right corner
      if((dy == 1) && (y == row2)) {
        dx = -1;
        dy = 0;
        if(line_type == 1) {
          c = 217;
        } else if(line_type == 2) {
          c = 188;
        }
      }
      // Change direction at bottom left corner
      if((dx == -1) && (x == col1)) {
        dx = 0;
        dy = -1;
        if(line_type == 1) {
          c = 192;
        } else if(line_type == 2) {
          c = 200;
        }
      }
      // Check for the top left corner
      if((dy == -1) && (y == row1)) {
        if(line_type == 1) {
          c = 218;
        } else if(line_type == 2) {
          c = 201;
        }
      }
      // Put new character to video
      vga4bit.textxy(x, y);
      vga4bit.write(c);
      // Move to next position
      x += dx;
      y += dy;
  }while((dy != -1) || (y >= row1));
}

