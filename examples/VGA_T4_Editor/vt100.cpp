/****************************************************************************************
 vt100.c

	vt100 decoder for the VT100 Terminal program


	Copyright (C) 2014 Geoff Graham (projects@geoffg.net)
	All rights reserved.

	This file and the program created from it are FREE FOR COMMERCIAL AND
	NON-COMMERCIAL USE as long as the following conditions are aheared to.

	Copyright remains Geoff Graham's, and as such any Copyright notices in the
	code are not to be removed.  If this code is used in a product, Geoff Graham
	should be given attribution as the author of the parts used.  This can be in
	the form of a textual message at program startup or in documentation (online
	or textual) provided with the program or product.

	Redistribution and use in source and binary forms, with or without modification,
	are permitted provided that the following conditions  are met:
	1. Redistributions of source code must retain the copyright notice, this list
	   of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright notice, this
	   list of conditions and the following disclaimer in the documentation and/or
	   other materials provided with the distribution.
	3. All advertising materials mentioning features or use of this software must
	   display the following acknowledgement:
	   This product includes software developed by Geoff Graham (projects@geoffg.net)

	THIS SOFTWARE IS PROVIDED BY GEOFF GRAHAM ``AS IS'' AND  ANY EXPRESS OR IMPLIED
	WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
	MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT
	SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
	IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGE.

	The licence and distribution terms for any publically available version or
	derivative of this code cannot be changed.  i.e. this code cannot simply be copied
	and put under another distribution licence (including the GNU Public Licence).
****************************************************************************************/
//============================================================
//= Modified for use with  VGA_4BIT_T4.                      =
//= Not all VT100 functions are implemented yet              =
//= Warren Watson 2017-2023                                  =
//============================================================

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "VGA_4bit_T4.h"
#include "vt100.h"

extern FlexIO2VGA vga4bit;

#define VTBUF_SIZE  40
unsigned char vtbuf[VTBUF_SIZE + 1] DMAMEM; // buffer for chars waiting to be decoded.
int vtcnt;                                  // count of the number of chars in vtbuf.
int arg[8], argc;                           // arguments to a command.
uint16_t CharPosX, CharPosY;                // current cursor coors.

// structure of the command table.
struct s_cmdtbl {
    char *name;          // the string
    int mode;            // 1 = ANSI only, 2 = VT52, 3 = both
    void (*fptr)(void);  // pointer to the function that will interpret that command
};

int CmdTblSize;
extern const struct s_cmdtbl PROGMEM cmdtbl[];

int mode; // current mode.  can be VT100 or VT52

// Attributes that can be turned on/off
int AttribUL, AttribRV, AttribInvis;
// saved attributes that can be restored
int SaveX, SaveY, SaveUL, SaveRV, SaveInvis, SaveFontNbr;
uint16_t SaveRVFGC, SaveRVBGC, SaveFGC, SaveBGC, DefaultFGC, DefaultBGC;

static const uint16_t PROGMEM vtColors[] = {
	VGA_BLACK,          // black
	VGA_BRIGHT_RED,     // red
	VGA_BRIGHT_GREEN,   // green
	VGA_BRIGHT_YELLOW,  // yellow
	VGA_BRIGHT_BLUE,    // blue
	VGA_BRIGHT_MAGENTA, // magenta
	VGA_BRIGHT_CYAN,    // cyan
	VGA_BRIGHT_WHITE    // white
};

// 
void VT100Putc(char c) {
  int cmd, i, j, partial;

  if(vtcnt >= VTBUF_SIZE) return;
  vtbuf[vtcnt++] = c;
  partial = false;
  while(vtcnt) {
    for(cmd = 0; cmd < CmdTblSize; cmd++) {
      if((cmdtbl[cmd].mode & mode) && *cmdtbl[cmd].name == *vtbuf) {   // check the mode then a quick check of the first char
       arg[0] = argc = 0;
         for(j = i = 1; cmdtbl[cmd].name[i] && j < vtcnt; i++, j++) {
           if(cmdtbl[cmd].name[i] == '^') {
            arg[argc++] = vtbuf[j] - 31;
           } else if(cmdtbl[cmd].name[i] == '@') {
             arg[argc] = 0;
             while(isdigit(vtbuf[j]))
               arg[argc] = arg[argc] * 10 + (vtbuf[j++] - '0');
             j--;   // correct for the overshoot, so that the next loop looks at the next char
             argc++;
           } else if(cmdtbl[cmd].name[i] != vtbuf[j])
             goto nextcmd; // compare failed, try the next command
         }
         if(cmdtbl[cmd].name[i] == 0) { // compare succeded, we have found the command
           vtcnt = 0; // clear all chars in the queue
           cmdtbl[cmd].fptr(); // and execute the command
           return;
         } else
           partial = true; // partial compare so set a flag to indicate this
      }
      nextcmd:
      continue;
    }
    if(!partial) { // we have searched the table and have not even found a partial match
      vga4bit.write(*vtbuf); // so send the oldest char off
      memcpy(vtbuf, vtbuf + 1, vtcnt--); // and shift down the table
    } else
      return; // have a partial match so keep the characters in the buffer
  } // keep looping until the buffer is empty
}

size_t vt100Write(char *p, size_t len) {
  size_t i;
  for(i = 0; i < len; i++) VT100Putc(p[i]);
  return len;
//    while(*p) vga4bit_print(*p++);
//    while(*p) VT100Putc(*p++);
}


void VideoPrintString(char *p) {
//    while(*p) vga4bit_print(*p++);
    while(*p) VT100Putc(*p++);
}

// utility function to move the cursor
void CursorPosition(int x, int y) {
	vga4bit.textxy(x,y);
	CharPosX = vga4bit.getTextX(); // update the horizontal character position
	CharPosY = vga4bit.getTextY(); // update the horizontal character position

}

// cursor up one or more lines
void cmd_CurUp(void) {
    if(argc == 0 || arg[0] == 0) arg[0] = 1;
    CursorPosition(CharPosX, CharPosY - arg[0]);
}

// cursor down one or more lines
void cmd_CurDown(void) {
    if(argc == 0 || arg[0] == 0) arg[0] = 1;
    CursorPosition(CharPosX, CharPosY + arg[0]);
}

// cursor left one or more chars
void cmd_CurLeft(void) {
    if(argc == 0 || arg[0] == 0) arg[0] = 1;
    CursorPosition(CharPosX - arg[0], CharPosY);
}

// cursor right one or more chars
void cmd_CurRight(void) {
    if(argc == 0 || arg[0] == 0) arg[0] = 1;
    CursorPosition(CharPosX + arg[0], CharPosY);
}

// cursor home
void cmd_CurHome(void) {
    CursorPosition(0, 0);
}

// position cursor
void cmd_CurPosition(void) {
    if(argc < 1 || arg[0] == 0) arg[0] = 0;
    if(argc < 2 || arg[1] == 0) arg[1] = 0;
    CursorPosition(arg[1]-1, arg[0]-1);  // note that the argument order is Y, X
}

// enter VT52 mode
void cmd_VT52mode(void) {
    mode = VT52;
}

// enter VT100 mode
void cmd_VT100mode(void) {
    mode = VT100;
}

// clear to end of line
void cmd_ClearEOL(void) {
	vga4bit.clreol();
}

// Clear the screen
void ClearScreen(void) {
	vga4bit.clear(vga4bit.getTextBGC());
}

// clear to end of screen
void cmd_ClearEOS(void) {
	vga4bit.clreos();
}

// clear from the beginning of the line to the cursor
void cmd_ClearBOL(void) {
	vga4bit.clrbol();
}

// clear from home to the cursor
void cmd_ClearBOS(void) {
	vga4bit.clrbos();
}

// turn the cursor off
void cmd_CursorOff(void) {
	vga4bit.cursorOff();
}

// turn the cursor on
void cmd_CursorOn(void) {
    vga4bit.cursorOn();
}

// save the current attributes
void cmd_ClearLine(void) {
	vga4bit.clrlin();
}

// save the current attributes
void cmd_CurSave(void) {
    SaveX = CharPosX;
    SaveY = CharPosY;
    SaveUL = AttribUL;
    SaveRV = AttribRV;
    SaveInvis = AttribInvis;
//    SaveFontNbr = fontNbr;
}

// restore the saved attributes
void cmd_CurRestore(void) {
    if(SaveFontNbr == -1) return;
    CursorPosition(SaveX, SaveY);
    AttribUL = SaveUL;
    AttribRV = SaveRV;
    AttribInvis = SaveInvis;
//    initFont(SaveFontNbr);
}

// set the keyboard to numbers mode
void cmd_SetNumLock(void) {
//    NumLock = false;
//    setLEDs(CapsLock,  false, 0);
}

// set the keyboard to arrows mode
void cmd_ExitNumLock(void) {
//    NumLock = true;
//    setLEDs(CapsLock,  true, 0);
}

// respond as a VT52
void cmd_VT52ID(void) {
    VideoPrintString((char *)"\033[/Z");
}

// respond as a VT100 thats OK
void cmd_VT100OK(void) {
    VideoPrintString((char *)"\033[0n");
}

// respond as a VT100 with no optiond
void cmd_VT100ID(void) {
    VideoPrintString((char *)"\033[?1;0c");    // vt100 with no options
}

// reset the terminal
void cmd_Reset(void) {
//    mode = VT100;
//    initFont(1);
//    ConfigBuffers(Option[O_LINES24]);
//    ShowCursor(false); // turn off the cursor to prevent it from getting confused
//    ClearScreen();
//    CursorPosition(1, 1);
//    AttribUL = 0;
//    AttribRV = 0;
//    AttribInvis = 0;
//    SaveFontNbr = -1;
}

// control the keyboard leds
void cmd_LEDs(void) {
//    static int led1 = 0;
//    static int led2 = 0;
//    static int led3 = 0;

//    if(arg[0] == 0) led1 = led2 = led3 = 0;
//    if(arg[0] == 2) led1 = 1;
//    if(arg[0] == 1) led2 = 1;
//    if(arg[0] == 3) led3 = 1;
//    setLEDs(led1, led2, led3);
}

// set attributes
void cmd_Attributes(void) {
  if(arg[0] == 0) {
    AttribUL = AttribRV = AttribInvis = 0;
	vga4bit.textColor(SaveFGC, SaveBGC);
	reverseVideo();
  }

  if(arg[0] == 7) {
	AttribRV = 1;
	reverseVideo();
  }

  if(arg[0] >= 30 && arg[0] < 38){ // fg colors
	SaveFGC = vga4bit.getTextFGC();
	vga4bit.textColor(vtColors[arg[0]-30], vga4bit.getTextBGC());
  } else if(arg[0] >= 40 && arg[0] < 48){ // bg color
	SaveBGC = vga4bit.getTextBGC();
	vga4bit.textColor(vga4bit.getTextFGC(), vtColors[arg[0]-40]);
  }

  if(arg[0] == 39){ // Default fg color
	vga4bit.textColor(DefaultFGC, vga4bit.getTextBGC());
  } else if(arg[0] == 49) { // Default bg color
	vga4bit.textColor(vga4bit.getTextFGC(), DefaultBGC);
  }
}

// Do reverse video.
void reverseVideo(void) {
  if(AttribRV == 1) {
    SaveRVFGC = vga4bit.getTextFGC();
    SaveRVBGC = vga4bit.getTextBGC();
    vga4bit.textColor(SaveRVBGC, SaveRVFGC);
  } else {
    vga4bit.textColor(SaveRVFGC, SaveRVBGC);
  }
}

// respond with the current vt100 cursor position (Not RA8876 coords)
void cmd_ReportPosition(void) {
  char s[20];
  sprintf(s, "\033[%d;%dR", CharPosY, CharPosX);
  VideoPrintString(s);
}

// do a line feed
void cmd_Lf(void) {
  vga4bit.write('\n');
}

// do a line feed
void cmd_LineFeed(void) {
  vga4bit.scrollUp();
}

// do an upwards line feed with a reverse scroll
void cmd_ReverseLineFeed(void) {
  if(CharPosY > 1)
    CursorPosition(CharPosX, CharPosY - 1);
  else
    vga4bit.scrollDown();
}

// turn on automatic line wrap, etc
void cmd_SetMode(void) {
//    if(arg[0] == 7) AutoLineWrap = true;
//    if(arg[0] == 9 && vga) {
//        cmd_CurHome();
//        ConfigBuffers(true);
//        ShowCursor(false); // turn off the cursor to prevent it from getting confused
//        ClearScreen();
//        CursorPosition(1, 1);
//    }
}

// turn off automatic line wrap, etc
void cmd_ResetMode(void) {
//    if(arg[0] == 7) AutoLineWrap = false;
//    if(arg[0] == 9 && vga) {
//        ConfigBuffers(false);
//        ShowCursor(false); // turn off the cursor to prevent it from getting confused
//        ClearScreen();
//        CursorPosition(1, 1);
//    }
}

// draw graphics
void cmd_Draw(void) {
//    if(Display24Lines) {
//        arg[2] = (arg[2]/3)*2;
//        arg[4] = (arg[4]/3)*2;
//    }
//    if(arg[0] == 1) DrawLine(arg[1], arg[2], arg[3], arg[4], 1);
//    if(arg[0] == 2) DrawBox(arg[1], arg[2], arg[3], arg[4], 0, 1);
//    if(arg[0] == 3) DrawBox(arg[1], arg[2], arg[3], arg[4], 1, 1);
//    if(arg[0] == 4) DrawCircle(arg[1], arg[2], arg[3], 0, 1, vga ? 1.14 : 1.0);
//    if(arg[0] == 5) DrawCircle(arg[1], arg[2], arg[3], 1, 1, vga ? 1.14 : 1.0);
}

// do nothing for escape sequences that are not implemented
void cmd_NULL(void) {}

/***********************************************************************
 The command table
 This is scanned from top to bottom by VT100Putc()
 Characters are matched exactly (ie, case sensitive) except for the '@'
 character which is a wild card character representing zero or more
 decimal characters of an argument and ^ which represents a single char
 and the value returned is the char - 31.                              
***********************************************************************/
const struct s_cmdtbl PROGMEM cmdtbl[]  = {
    { (char *)"\033A",          VT52,       cmd_CurUp },
    { (char *)"\033B",          VT52,       cmd_CurDown },
    { (char *)"\033C",          VT52,       cmd_CurRight },
    { (char *)"\033D",          VT52,       cmd_CurLeft },
    { (char *)"\033H",          VT52,       cmd_CurHome },
    { (char *)"\033I",          VT52,       cmd_ReverseLineFeed },
    { (char *)"\033[^^",        VT52,       cmd_CurPosition },

    { (char *)"\033J",          VT52,       cmd_ClearEOS },
    { (char *)"\033K",          VT52,       cmd_ClearEOL },

    { (char *)"\033>",          VT52,       cmd_SetNumLock },
    { (char *)"\033=",          VT52,       cmd_ExitNumLock },

    { (char *)"\033[Z",         VT52,       cmd_VT52ID },
    { (char *)"\033<",          VT52,       cmd_VT100mode },
    { (char *)"\033F",          VT52,       cmd_NULL },
    { (char *)"\033G",          VT52,       cmd_NULL },

    { (char *)"\033[K",         VT100,      cmd_ClearEOL },
    { (char *)"\033[J",         VT100,      cmd_ClearEOS },
    { (char *)"\033[0K",        VT100,      cmd_ClearEOL },
    { (char *)"\033[1K",        VT100,      cmd_ClearBOL },
    { (char *)"\033[2K",        VT100,      cmd_ClearLine },
    { (char *)"\033[0J",        VT100,      cmd_ClearEOS },
    { (char *)"\033[1J",        VT100,      cmd_ClearBOS },
    { (char *)"\033[2J",        VT100,      ClearScreen },

    { (char *)"\033[?25l",      VT100,      cmd_CursorOff },
    { (char *)"\033[?25h",      VT100,      cmd_CursorOn },

    { (char *)"\033[@A",        VT100,      cmd_CurUp },
    { (char *)"\033[@B",        VT100,      cmd_CurDown },
    { (char *)"\033[@C",        VT100,      cmd_CurRight },
    { (char *)"\033[@D",        VT100,      cmd_CurLeft },
    { (char *)"\033[@;@H",      VT100,      cmd_CurPosition },
    { (char *)"\033[@;@f",      VT100,      cmd_CurPosition },
    { (char *)"\033[H",         VT100,      cmd_CurHome },
    { (char *)"\033[f",         VT100,      cmd_CurHome },
    { (char *)"\0337",          VT100,      cmd_CurSave },
    { (char *)"\0338",          VT100,      cmd_CurRestore },
    { (char *)"\0336n",         VT100,      cmd_ReportPosition },

    { (char *)"\033D",          VT100,      cmd_LineFeed },
    { (char *)"\033M",          VT100,      cmd_ReverseLineFeed },
    { (char *)"\033E",          VT100,      cmd_Lf },
    { (char *)"\033[?@h",       VT100,      cmd_SetMode },
    { (char *)"\033[?@l",       VT100,      cmd_ResetMode },

    { (char *)"\033[>",         VT100,      cmd_SetNumLock },
    { (char *)"\033[=",         VT100,      cmd_ExitNumLock },

    { (char *)"\033[Z@;@;@;@;@Z",VT100,     cmd_Draw },
    { (char *)"\033[Z@;@;@;@Z",  VT100,     cmd_Draw },

    { (char *)"\033[@m" ,       VT100,      cmd_Attributes },
    { (char *)"\033[@q" ,       VT100,      cmd_LEDs },
    { (char *)"\033c" ,         BOTH,       cmd_Reset },
    { (char *)"\033[c" ,        VT100,      cmd_VT100ID },
    { (char *)"\033[0c" ,       VT100,      cmd_VT100ID },
    { (char *)"\033[5n" ,       VT100,      cmd_VT100OK },
    { (char *)"\033[?2l",       VT100,      cmd_VT52mode },
};    // cmd_VT52mode must be last in the table.

/*
  [12;24r   Set scrolling region to lines 12 thru 24.  If a linefeed or an
            INDex is received while on line 24, the former line 12 is deleted
            and rows 13-24 move up.  If a RI (reverse Index) is received while
            on line 12, a blank line is inserted there as rows 12-13 move down.
            All VT100 compatible terminals (except GIGI) have this feature.

*/
//=========================
// Initialize VT100 system.
//=========================
void initVT100(void) {
    mode = VT100;
    vtcnt = 0;
    CharPosX = 0;
    CharPosY = 0;
    CmdTblSize =  (sizeof(cmdtbl)/sizeof(struct s_cmdtbl));
    SaveFontNbr = -1;
	DefaultFGC = SaveFGC = vga4bit.getTextFGC();
	DefaultBGC = SaveBGC = vga4bit.getTextBGC();
}
