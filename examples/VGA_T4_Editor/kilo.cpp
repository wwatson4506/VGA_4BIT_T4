// Teensy kilo Editor

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "USBKeyboard.h"
#include "VGA_4bit_T4.h"
#include "VGA_T4_Config.h"
#include "vt100.h"
#include "kilo.h"

/*** defines ***/

#define KILO_VERSION "0.0.2"
#define KILO_TAB_STOP 2
#define KILO_QUIT_TIMES 2

#define CTRL_KEY(k) ((k) & 0x1f)

static 	char *msgBuffer;
static bool exitflag;

extern FlexIO2VGA vga4bit;

FS *fsType = nullptr;

// Array of RA8876 Basic Colors
const uint16_t kiloColors[] = {
  VGA_BLACK,          // 0
  VGA_BLUE,           // 1
  VGA_GREEN,          // 2
  VGA_CYAN,           // 3
  VGA_RED,            // 4
  VGA_MAGENTA,        // 5
  VGA_YELLOW,         // 6
  VGA_WHITE,          // 7
  VGA_GREY,           // 8
  VGA_BRIGHT_BLUE,    // 9
  VGA_BRIGHT_GREEN,   // 10
  VGA_BRIGHT_CYAN,    // 11
  VGA_BRIGHT_RED,     // 12
  VGA_BRIGHT_MAGENTA, // 13
  VGA_BRIGHT_YELLOW,  // 14
  VGA_BRIGHT_WHITE    // 15
};

// Keyboard special Keys
#define KEYD_UP    		0xDA
#define KEYD_DOWN    	0xD9
#define KEYD_LEFT   	0xD8
#define KEYD_RIGHT   	0xD7
#define KEYD_INSERT		0xD1
#define KEYD_DELETE		0xD4
#define KEYD_PAGE_UP    0xD3
#define KEYD_PAGE_DOWN  0xD6
#define KEYD_HOME      	0xD2
#define KEYD_END       	0xD5
#define KEYD_F1        	0xC2
#define KEYD_F2        	0xC3
#define KEYD_F3        	0xC4
#define KEYD_F4        	0xC5
#define KEYD_F5        	0xC6
#define KEYD_F6        	0xC7
#define KEYD_F7        	0xC8
#define KEYD_F8        	0xC9
#define KEYD_F9        	0xCA
#define KEYD_F10       	0xCB
#define KEYD_F11       	0xCC
#define KEYD_F12       	0xCD
#define KEYD_BACKSPACE  0x7F

File fp;

enum editorKey {
  BACKSPACE = 127,
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

enum editorHighlight {
  HL_NORMAL = 0,
  HL_COMMENT,
  HL_MLCOMMENT,
  HL_KEYWORD1,
  HL_KEYWORD2,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH
};

#define HL_HIGHLIGHT_NUMBERS (1<<0)
#define HL_HIGHLIGHT_STRINGS (1<<1)

/*** data ***/

struct editorSyntax {
  const char *filetype;
  const char **filematch;
  const char **keywords;
  const char *singleline_comment_start;
  const char *multiline_comment_start;
  const char *multiline_comment_end;
  int flags;
};

typedef struct erow {
  int idx;
  int size;
  int rsize;
  char *chars;
  char *render;
  unsigned char *hl;
  int hl_open_comment;
} erow;

struct editorConfig {
  int cx, cy;
  int rx;
  int rowoff;
  int coloff;
  int screenrows;
  int screencols;
  int numrows;
  erow *row;
  int dirty;
  char *filename;
  char statusmsg[80];
  time_t statusmsg_time;
  struct editorSyntax *syntax;
};

struct editorConfig E;

/*** filetypes ***/

PROGMEM const char *C_HL_extensions[] = { ".c", ".h", ".cpp", ".ino", NULL };
PROGMEM const char *C_HL_keywords[] = {
  "switch", "if", "while", "for", "break", "continue", "return", "else",
  "struct", "union", "typedef", "static", "enum", "class", "case",

  "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
  "void|", NULL
};

PROGMEM const char *BAS_HL_extensions[] = { ".bas", ".lib", NULL };
PROGMEM const char *BAS_HL_keywords[] = {
  "print", "input", "if", "while", "for", "then", "next", "return", "else",
  "goto", "gosub", "list", "load", "save", "elseif", "wend", "tcursor", "gcursor",
  "gcursorxy", "gcursorset", "tcursor", "defcursor", "tcursorfgc", "gcursorfgc",
  "tcblink", "gbuttoninit", "drawgbutton", "chdrv", "beep", "commput", "rtcset",
  "read", "data", "restore", "on", "dim", "erase", "test", "edit", "help", "run",
  "new", "system", "pause", "tron", "troff", "cls", "get", "put", "poke",
  "adcinit", "pinmode", "pwm", "pwmfreq", "shiftout", "eewrite", "slcls",
  "slprint", "endif", "pwminit", "end", "repeat", "until", "to", "arrayfill",
  "swap", "clr", "let", "lset", "rset", "open", "close", "procedure", "local",
  "function", "exitif", "stop", "do", "loop","llist", "exit", "downto", "step",
  "color", "plot", "line", "draw", "circle", "ellipse", "dir", "cd", "kill",
  "seek", "md", "copy", "rename", "fontsize", "pinwrite",
  "rectangle", "rrectangle", "triangle", "drawbutton",
  "tspoint", "picture", "fvline", "tab", "timeon", "timeoff", 
  "vmemput", "boxput", "boxget", "margins", "scrollup", "scrolldn", "opendir",
  "closedir", "readdir", "clear", "fontsource", "inc","dec", "pip", "screen",
  
  "int|", "%|", "at|", "using|", "hex$|", "pinread|", "inkey$|",
  "chr$|", "adc|", "width|", "height|", "and|", "or|", "xor|", "not|", "mod|",
  "sqa|", "sqr|", "sin|", "cos|", "tan|", "atn|", "log|", "log10|", "exp|",
  "abs|", "sgn|", "frac|", "odd|", "trunc|", "random|", "add|", "sub|",
  "mul|", "div|", "pi|", "true|", "false|", "min|", "max|", "timer|",
  "time$|", "date$|", "exist|", "str$|", "val|", "asc|", "len|", "mid$|",
  "left$|", "right$|", "upper$|", "lower$|", "space$|", "string$|", "!|",
  "deffn|", "eof|", "lof|", "dfree|", "mfree|", "rnd|", "varptr|", "peek|",
  "hex|", "bin$|", "bin|", "eeread|",  "shiftin|", "xpos|", "ypos|", "gwidth|",
  "gheight|", "fgcolor|", "bgcolor|", "mousex|", "mousey|", "mouseb|", 
  "mousew|", "gblocate|", "tstouch|", "getpixel|",  "commget|", "map|", "loc|",
  "ltrim$|", "rtrim$|", "pwd$|", "fontheight|", "fontwidth|", "scancode|",
  "modifiers|", "shl|", "shr|", "jsbuttons|", "jsavailable|", "jsaxis|", "mouse|", "vmemget|",
  "micros|", "instr$|",NULL
};

struct editorSyntax HLDB[] = {
  {
    "bas",
    BAS_HL_extensions,
    BAS_HL_keywords,
    "'", "rem", "'",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
  {
    "c",
    C_HL_extensions,
    C_HL_keywords,
    "//", "/*", "*/",
    HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  }
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))
static volatile uint32_t memusage = 0;

/*** prototypes ***/

void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(const char *prompt, void (*callback)(char *, int));
int32_t getFileStr(char *line);
uint32_t freeram(void);

uint32_t freeram(void) {
 	unsigned long stackTop;
	unsigned long heapTop;
	
	// Current position of stack.
	stackTop = (unsigned long) &stackTop;
	
	// Current position of heap.
	void *hTop = malloc(1);
	heapTop = (unsigned long) hTop;
	free(hTop);
	
	// The difference is the free, available ram.
	return (uint32_t)((0x20280000 - heapTop) - memusage);
}

/*** terminal ***/
uint16_t editorReadKey() {

	uint16_t result = 0;
	while(!USBKeyboard_available());
	result = (uint16_t)USBKeyboard_read();
	if(result == '\n')
		result = '\r';
	return result;
}

void die(const char *s) {
  vt100Write((char *)"\x1b[2J", 4); // clear screen
  vt100Write((char *)"\x1b[H", 3); // home cursor

  vga4bit.printf("%s\n",s);
  exitflag = true;
  vga4bit.printf("\nPress any key to continue...\n");
  editorReadKey();
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  if(vt100Write((char *)"\x1b[6n", 4) != 4) return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  return 0;
}

int getWindowSize(int *rows, int *cols) {
    *cols = vga4bit.getTwidth();
    *rows = vga4bit.getTheight();
    return 0;
}

/*** syntax highlighting ***/

int is_separator(int c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void tolowerStr(char *str) {
	for(int i = 0; str[i]; i++){
		str[i] = tolower(str[i]);
	}
}

void editorUpdateSyntax(erow *row) {
  row->hl = (unsigned char *)realloc(row->hl, row->rsize);
  memset(row->hl, HL_NORMAL, row->rsize);

  if (E.syntax == NULL) return;

  const char **keywords = E.syntax->keywords;

  const char *scs = E.syntax->singleline_comment_start;
  const char *mcs = E.syntax->multiline_comment_start;
  const char *mce = E.syntax->multiline_comment_end;

  int scs_len = scs ? strlen(scs) : 0;
  int mcs_len = mcs ? strlen(mcs) : 0;
  int mce_len = mce ? strlen(mce) : 0;

  int prev_sep = 1;
  int in_string = 0;
  int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);

  int i = 0;
  while (i < row->rsize) {
    char c = row->render[i];
    unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;

    if (scs_len && !in_string && !in_comment) {
      if (!strncmp(&row->render[i], scs, scs_len)) {
        memset(&row->hl[i], HL_COMMENT, row->rsize - i);
        break;
      }
    }

    if (mcs_len && mce_len && !in_string) {
      if (in_comment) {
        row->hl[i] = HL_MLCOMMENT;
        if (!strncmp(&row->render[i], mce, mce_len)) {
          memset(&row->hl[i], HL_MLCOMMENT, mce_len);
          i += mce_len;
          in_comment = 0;
          prev_sep = 1;
          continue;
        } else {
          i++;
          continue;
        }
      } else if (!strncmp(&row->render[i], mcs, mcs_len)) {
        memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
        i += mcs_len;
        in_comment = 1;
        continue;
      }
    }

    if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
      if (in_string) {
        row->hl[i] = HL_STRING;
        if (c == '\\' && i + 1 < row->rsize) {
          row->hl[i + 1] = HL_STRING;
          i += 2;
          continue;
        }
        if (c == in_string) in_string = 0;
        i++;
        prev_sep = 1;
        continue;
      } else {
        if (c == '"' || c == '\'') {
          in_string = c;
          row->hl[i] = HL_STRING;
          i++;
          continue;
        }
      }
    }

    if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
      if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
          (c == '.' && prev_hl == HL_NUMBER)) {
        row->hl[i] = HL_NUMBER;
        i++;
        prev_sep = 0;
        continue;
      }
    }

    if (prev_sep) {
      int j;
      for (j = 0; keywords[j]; j++) {
        int klen = strlen(keywords[j]);
        int kw2 = keywords[j][klen - 1] == '|';
        if (kw2) klen--;

        if (!strncmp(&row->render[i], keywords[j], klen) &&
            is_separator(row->render[i + klen])) {
          memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
          i += klen;
          break;
        }
      }
      if (keywords[j] != NULL) {
        prev_sep = 0;
        continue;
      }
    }

    prev_sep = is_separator(c);
    i++;
  }

  int changed = (row->hl_open_comment != in_comment);
  row->hl_open_comment = in_comment;
  if (changed && row->idx + 1 < E.numrows)
    editorUpdateSyntax(&E.row[row->idx + 1]);
}

int editorSyntaxToColor(int hl) {
  switch (hl) {
    case HL_COMMENT:
    case HL_MLCOMMENT: return 36;
    case HL_KEYWORD1: return 33;
    case HL_KEYWORD2: return 32;
    case HL_STRING: return 35;
    case HL_NUMBER: return 31;
    case HL_MATCH: return 34;
    default: return 37;
  }
}

void editorSelectSyntaxHighlight() {
  E.syntax = NULL;
  if (E.filename == NULL) return;

  char *ext = strrchr(E.filename, '.');

  for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
    struct editorSyntax *s = &HLDB[j];
    unsigned int i = 0;
    while (s->filematch[i]) {
      int is_ext = (s->filematch[i][0] == '.');
      if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
          (!is_ext && strstr(E.filename, s->filematch[i]))) {
        E.syntax = s;

        int filerow;
        for (filerow = 0; filerow < E.numrows; filerow++) {
          editorUpdateSyntax(&E.row[filerow]);
        }

        return;
      }
      i++;
    }
  }
}

/*** row operations ***/

int editorRowCxToRx(erow *row, int cx) {
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
    rx++;
  }
  return rx;
}

int editorRowRxToCx(erow *row, int rx) {
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size; cx++) {
    if (row->chars[cx] == '\t')
      cur_rx += (KILO_TAB_STOP - 1) - (cur_rx % KILO_TAB_STOP);
    cur_rx++;

    if (cur_rx > rx) return cx;
  }
  return cx;
}

void editorUpdateRow(erow *row) {
  int tabs = 0;
  int j;
  for (j = 0; j < row->size; j++)
    if (row->chars[j] == '\t') tabs++;

  free(row->render);
  row->render = (char *)malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);

  int idx = 0;
  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % KILO_TAB_STOP != 0) row->render[idx++] = ' ';
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = idx;

  editorUpdateSyntax(row);
}

void editorInsertRow(int at, const char *s, size_t len) {
  if (at < 0 || at > E.numrows) return;

  E.row = (erow *)realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));
  for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;

  E.row[at].idx = at;

  E.row[at].size = len;
  E.row[at].chars = (char *)malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';

  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  E.row[at].hl = NULL;
  E.row[at].hl_open_comment = 0;
  editorUpdateRow(&E.row[at]);

  E.numrows++;
  E.dirty++;
}

void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
  free(row->hl);
}

void editorDelRow(int at) {
  if (at < 0 || at >= E.numrows) return;
  editorFreeRow(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  for (int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
  E.numrows--;
  E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size) at = row->size;
  row->chars = (char *)realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = (char *)realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowDelChar(erow *row, int at) {
  if (at < 0 || at >= row->size) return;
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  editorUpdateRow(row);
  E.dirty++;
}

/*** editor operations ***/

void editorInsertChar(int c) {
  if (E.cy == E.numrows) {
    editorInsertRow(E.numrows, "", 0);
  }
  editorRowInsertChar(&E.row[E.cy], E.cx, c);
  E.cx++;
}

void editorInsertNewline() {
  if (E.cx == 0) {
    editorInsertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }
  E.cy++;
  E.cx = 0;
}

void editorDelChar() {
  if (E.cy == E.numrows) return;
  if (E.cx == 0 && E.cy == 0) return;

  erow *row = &E.row[E.cy];
  if (E.cx > 0) {
    editorRowDelChar(row, E.cx - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
    editorDelRow(E.cy);
    E.cy--;
  }
}

/*** file i/o ***/
//================================================
// Get a '\n' terminated string from an open file.
// fgets() sort of... (for testing getline();
//================================================
int32_t getFileStr(char *line) {
	for(int i = 0; i < 256; i++) {
		if(fp.available()) {
			line[i] = fp.read();
			if(line[i] == '\n') return strlen(line);
		} else return -1;
	}
    return -1;
}

/* Read up to (and including) a newline from STREAM into *LINEPTR
   (and null-terminate it). *LINEPTR is a pointer returned from malloc (or
   NULL), pointing to *N characters of space.  It is realloc'd as
   necessary.  Returns the number of characters read (not including the
   null terminator), or -1 on error or EOF.  */
int getline(char **lineptr, size_t *n)
{
  char line[256];
  char *ptr;
  unsigned int len;
  
   if (lineptr == NULL || n == NULL)
   {
      errno = EINVAL;
      return -1;
   }
    for(int i = 0; i < 256; i++) {
		if(fp.available()) {
			line[i] = fp.read();
			if(line[i] == '\n') break; // End of line
		} else return -1; // End of file
	}

   ptr = strchr(line,'\n');   
   if (ptr)
      *ptr = '\0';

   len = strlen(line);
   
   if ((len+1) < 256)
   {
      ptr = (char *)realloc(*lineptr, 256);
      if (ptr == NULL)
         return(-1);
      *lineptr = ptr;
      *n = 256;
   }

   strcpy(*lineptr,line); 
   return(len);
}

int editorOpen(char *filename) {
  
  E.dirty = 0;
  free(E.filename);
  
  size_t fnlen = strlen(filename)+1;
  E.filename = (char *)malloc(fnlen);
  memcpy(E.filename,filename,fnlen);

  editorSelectSyntaxHighlight();

  fp = fsType->open(filename,FILE_READ);
  if (!fp) {
	fp = fsType->open(E.filename, FILE_WRITE_BEGIN);
	fp.close();
    fp = fsType->open(filename,FILE_READ);
  }
  if(fp.size() > (freeram()-50000)) {  // Not sure...
	fp.close();
	return -2;
  }
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;

  while ((linelen = getline(&line, &linecap)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
      linelen--;
    editorInsertRow(E.numrows, line, linelen);
  }
  free(line);
  fp.close();
  E.dirty = 0;
  return 0;
}

void editorSave(void) {
  int j, len = 0;
  char tempFilename[256];
  char buf[256];

  for (j = 0; j < E.numrows; j++)
    len += E.row[j].size + 1;

  if (E.filename == NULL) {
    E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
    if (E.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }
    editorSelectSyntaxHighlight();
  }
  sprintf(tempFilename, "%s", E.filename);
  fp = fsType->open(tempFilename, FILE_WRITE_BEGIN);
  if (fp) {
	for (j = 0; j < E.numrows; j++) {
		memcpy(buf, E.row[j].chars, E.row[j].size);
		buf[E.row[j].size] = '\n';
		buf[E.row[j].size+1] = '\0';
        fp.write(buf,strlen(buf));
	}	
	fp.truncate(fp.position());
	fp.close();
	E.dirty = 0;
	editorSetStatusMessage("%d bytes written to disk", len);
	return;
  }
  fp.close();
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(EIO));
}

/*** find ***/

void editorFindCallback(char *query, int key) {
  static int last_match = -1;
  static int direction = 1;

  static int saved_hl_line;
  static char *saved_hl = NULL;

  if (saved_hl) {
    memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
    free(saved_hl);
    saved_hl = NULL;
  }

  if (key == '\r' || key == '\x1b') {
    last_match = -1;
    direction = 1;
    return;
  } else if (key == KEYD_RIGHT || key == KEYD_DOWN) {
    direction = 1;
  } else if (key == KEYD_LEFT || key == KEYD_UP) {
    direction = -1;
  } else {
    last_match = -1;
    direction = 1;
  }

  if (last_match == -1) direction = 1;
  int current = last_match;
  int i;
  for (i = 0; i < E.numrows; i++) {
    current += direction;
    if (current == -1) current = E.numrows - 1;
    else if (current == E.numrows) current = 0;

    erow *row = &E.row[current];
    char *match = strstr(row->render, query);
    if (match) {
      last_match = current;
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render);
      E.rowoff = E.numrows;

      saved_hl_line = current;
      saved_hl = (char *)malloc(row->rsize);
      memcpy(saved_hl, row->hl, row->rsize);
      memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
      break;
    }
  }
}

void editorFind() {
  int saved_cx = E.cx;
  int saved_cy = E.cy;
  int saved_coloff = E.coloff;
  int saved_rowoff = E.rowoff;

  char *query = editorPrompt("Search: %s (Use ESC/Arrows/Enter)",
                             editorFindCallback);

  if (query) {
    free(query);
  } else {
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}

/*** append buffer ***/

struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new1 = (char *)realloc(ab->b, ab->len + len);

  if (new1 == NULL) return;
  memcpy(&new1[ab->len], s, len);
  ab->b = new1;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}

/*** output ***/

void editorScroll() {
  E.rx = 0;
  if (E.cy < E.numrows) {
    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
  }

  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  if (E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  if (E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    int filerow = y + E.rowoff;
    if (filerow >= E.numrows) {
      if (E.numrows == 0 && y == E.screenrows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome),
          "Kilo editor -- Teensy version %s", KILO_VERSION);
        if (welcomelen > E.screencols) welcomelen = E.screencols;
        int padding = (E.screencols - welcomelen) / 2;
        if (padding) {
          abAppend(ab, "~", 1);
          padding--;
        }
        while (padding--) abAppend(ab, " ", 1);
        abAppend(ab, welcome, welcomelen);
      } else {
        abAppend(ab, "~", 1);
      }
    } else {
      int len = E.row[filerow].rsize - E.coloff;
      if (len < 0) len = 0;
      if (len > E.screencols) len = E.screencols;
      char *c = &E.row[filerow].render[E.coloff];
      unsigned char *hl = &E.row[filerow].hl[E.coloff];
      int current_color = -1;
      int j;
      for (j = 0; j < len; j++) {
        if (iscntrl(c[j])) {
          char sym = (c[j] <= 26) ? '@' + c[j] : '?';
          abAppend(ab, "\x1b[7m", 4); // reverse video
          abAppend(ab, &sym, 1);
          abAppend(ab, "\x1b[m", 3);
          if (current_color != -1) {
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
            abAppend(ab, buf, clen);
          }
        } else if (hl[j] == HL_NORMAL) {
          if (current_color != -1) {
            abAppend(ab, "\x1b[39m", 5);
            current_color = -1;
          }
          abAppend(ab, &c[j], 1);
        } else {
          int color = editorSyntaxToColor(hl[j]);
          if (color != current_color) {
            current_color = color;
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
            abAppend(ab, buf, clen);
          }
          abAppend(ab, &c[j], 1);
        }
      }
      abAppend(ab, "\x1b[39m", 5);
    }

    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\r\n", 2);
  }
}

void editorDrawStatusBar(struct abuf *ab) {
  abAppend(ab, "\x1b[7m", 4); // Set Reverse video
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
    E.filename ? E.filename : "[No Name]", E.numrows,
    E.dirty ? "(modified)" : "");
  int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
    E.syntax ? E.syntax->filetype : "no ft", E.cy + 1, E.numrows);
  if (len > E.screencols) len = E.screencols;
  abAppend(ab, status, len);
  while (len < E.screencols) {
    if (E.screencols - len == rlen+1) {
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      abAppend(ab, " ", 1);
      len++;
    }
  }
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2);
}

void editorDrawMessageBar(struct abuf *ab) {
  abAppend(ab, "\x1b[K", 3); //Clear to end of line
  int msglen = strlen(E.statusmsg);
  if (msglen > E.screencols) msglen = E.screencols;
  if (msglen && (millis() - E.statusmsg_time) < 5000)
    abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen() {
  editorScroll();

  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6); // cursor off
  abAppend(&ab, "\x1b[H", 3); // home cursor

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);
  editorDrawMessageBar(&ab);

  char buf[32];

  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1,
                                            (E.rx - E.coloff) + 1);
  abAppend(&ab, buf, strlen(buf));

  abAppend(&ab, "\x1b[?25h", 6); //cursor on

  vt100Write(ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
  va_end(ap);
  E.statusmsg_time = millis();
}

/*** input ***/

char *editorPrompt(const char *prompt, void (*callback)(char *, int)) {
  size_t bufsize = 128;
  char *buf = (char *)malloc(bufsize);

  size_t buflen = 0;
  buf[0] = '\0';

  while (1) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();

    uint16_t c = editorReadKey();
    if (c == KEYD_DELETE || c == CTRL_KEY('h') || c == KEYD_BACKSPACE) {
      if (buflen != 0) buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      editorSetStatusMessage("");
      if (callback) callback(buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        editorSetStatusMessage("");
        if (callback) callback(buf, c);
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = (char *)realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }

    if (callback) callback(buf, c);
  }
}

void editorMoveCursor(int key) {
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];

  switch (key) {
    case KEYD_LEFT:
      if (E.cx != 0) {
        E.cx--;
      } else if (E.cy > 0) {
        E.cy--;
        E.cx = E.row[E.cy].size;
      }
      break;
    case KEYD_RIGHT:
      if (row && E.cx < row->size) {
        E.cx++;
      } else if (row && E.cx == row->size) {
        E.cy++;
        E.cx = 0;
      }
      break;
    case KEYD_UP:
      if (E.cy != 0) {
        E.cy--;
      }
      break;
    case KEYD_DOWN:
      if (E.cy < E.numrows) {
        E.cy++;
      }
      break;
  }

  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) {
    E.cx = rowlen;
  }
}

void editorProcessKeypress() {
  static int quit_times = KILO_QUIT_TIMES;

  uint16_t c = editorReadKey();
  switch (c) {
    case '\r':
      editorInsertNewline();
      break;

    case CTRL_KEY('q'):
    case KEYD_F10:
      if (E.dirty && quit_times > 0) {
        editorSetStatusMessage("WARNING!!! File has unsaved changes. "
          "Press Ctrl-Q %d more times to quit.", quit_times);
        quit_times--;
        return;
      }
      vt100Write((char *)"\x1b[2J", 4);
      vt100Write((char *)"\x1b[H", 3);
      exitflag = true;
      break;

    case CTRL_KEY('s'):
      editorSave();
      break;

    case KEYD_HOME:
      E.cx = 0;
      break;

    case KEYD_END:
      if (E.cy < E.numrows)
        E.cx = E.row[E.cy].size;
      break;

    case CTRL_KEY('f'):
      editorFind();
      break;

    case KEYD_BACKSPACE:
    case KEYD_DELETE:
      if (c == KEYD_DELETE) editorMoveCursor(KEYD_RIGHT);
      editorDelChar();
      break;

    case KEYD_PAGE_UP:
    case KEYD_PAGE_DOWN:
      {
        if (c == KEYD_PAGE_UP) {
          E.cy = E.rowoff;
        } else if (c == KEYD_PAGE_DOWN) {
          E.cy = E.rowoff + E.screenrows - 1;
          if (E.cy > E.numrows) E.cy = E.numrows;
        }

        int times = E.screenrows;
        while (times--)
          editorMoveCursor(c == KEYD_PAGE_UP ? KEYD_UP : KEYD_DOWN);
      }
      break;

    case KEYD_UP:
    case KEYD_DOWN:
    case KEYD_LEFT:
    case KEYD_RIGHT:
      editorMoveCursor(c);
      break;

    case CTRL_KEY('l'):
    case '\x1b':
      break;

    case KEYD_F1: 
		editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find | F1 = HELP");
        break;
        
    default:
      editorInsertChar(c);
      break;
  }

  quit_times = KILO_QUIT_TIMES;
}

/*** init ***/

void initEditor() {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  E.dirty = 0;
  E.filename = NULL;
  E.statusmsg[0] = '\0';
  E.statusmsg_time = 0;
  E.syntax = NULL;

  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize: Error.");
  E.screenrows -= 3; // Room for 2 info lines and 1 for available memory.
//  E.screencols -= 1; // Fix for missing col at right of screen.
  exitflag = false;
  vga4bit.initCursor(0,0,2,15,true,30); // Define I-beam text cursor.
  vga4bit.setCursorBlink(false);        // disable binking cursor.
  while(USBKeyboard_available());       // Clear keyboard buffer.
}

int kilo(FS *type, char *filename) {
  fsType = type;
  int err = 0;
  
  if(filename == NULL) return -1;

  msgBuffer = (char *)malloc(256);
  sprintf(msgBuffer, "%s %lu","Free RAM:", freeram());

  initEditor();
  editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find | F1 = HELP");
  if((err = editorOpen(filename)) == -2) return err;

  while (!exitflag) {
	vga4bit.slWrite(0, kiloColors[15], kiloColors[1], msgBuffer);
    editorRefreshScreen();
	vga4bit.slWrite(0, kiloColors[15], kiloColors[1], msgBuffer);
    editorProcessKeypress();
	vga4bit.slWrite(0, kiloColors[15], kiloColors[1], msgBuffer);
  }

  // Free allocated memory (Last row to First row)
  for(int i = E.numrows; i >= 0; i--) {
	  editorDelRow(i);
  }
  free(E.row);
  free(E.filename);
  free(msgBuffer);
  vga4bit.setCursorBlink(true); // enable binking cursor
  vga4bit.setCursorType(0);		 // use block cursor
  vga4bit.clearPrintWindow();
  
  return 0;
}
