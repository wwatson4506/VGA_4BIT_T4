#include "menu.h"
#include "box.h"

extern FlexIO2VGA vdc4bit;

// This should be in a struct. Maybe later:)
static int c_lines = VGA_BLACK;
static int c_hiletter = VGA_BRIGHT_WHITE;
static int c_hitext = VGA_WHITE;
static int c_text=VGA_BLACK;
static int c_back = VGA_WHITE;
static int c_title = VGA_BLACK;
static int c_prompt = VGA_BLACK;
static int c_hiback=VGA_BLACK;

// Ditto: Default border lines and shadow control
static int mb_lines = 1;
static int mb_shadow = 1;

// Change border line type (single/double).
void menu_box_lines(int line_type) {
	mb_lines = line_type;
}

// Turn shadow on or off.
void menu_shadow(int on_off) {
	mb_shadow = on_off;
}

// Change menu background color.
void menu_back(int back) {
	c_back = back;
}

// change menu borderline color.
void menu_line_color(int lines) {
	c_lines = lines;
}

// Change menu title color.
void menu_title(int title) {
	c_title = title;
}

// Change menu text color.
void menu_text_color(int text) {
	c_text = text;
}

// Change menu prompt color.
void menu_prompt(int prompt) {
	c_prompt = prompt;
}

// Change menu hilited letter color.
void menu_hiletter(int hiletter) {
	c_hiletter = hiletter;
}

// Change menu hilited text color.
void menu_hitext(int hitext) {
	c_hitext = hitext;
}

// Change menu hilited background color.
void menu_hilight_back(int hiback)
{
	c_hiback = hiback;
}

// Build the main menu bar.
uint8_t *menu_bar(int16_t row, int16_t col, const char *string, int *choice) {

	int	len;
	int	maxchoice;
	int	i, j;
	int	cpos = 0;
	int	quit_flag = 0;
    uint8_t *savebuf;
	int	fstr[81]; // Format string for each entry string.
	int	bstr[81]; // Background color for each entry string.
	char	lastc, thisc;
	char	key;
    
    //Save some color and postion info for later.
    uint8_t fColor = vga4bit.getTextFGC();  
    uint8_t bColor = vga4bit.getTextBGC();  
    uint8_t curx = vga4bit.getTextX();
    uint8_t cury = vga4bit.getTextY();

	// Get length of entry string.
	len  = strlen(string);

	// save the menu background
	if(mb_shadow)
		savebuf = (uint8_t *)vbox_get(row, col, row + 1, col + len + 1);
	else
		savebuf = (uint8_t *)vbox_get(row, col, row, col + len - 1);

	
	// Cast a shadow if enabled.
	if(mb_shadow) {
      vga4bit.setForegroundColor(VGA_GREY);
      vga4bit.setBackgroundColor(VGA_BLACK);
      box_charfill(row + 1, col + 2, row + 1, col + len + 1, B_PATTERN);
	}
    
    // Make sure choice is at a minimum of 1. (First menu bar entry)
	if (*choice < 1)
		*choice = 1;

	// process each keypress
	while(!quit_flag) {
		// determine color attributes
		j = 0;
		maxchoice = 0;
		lastc = 0;
		for(i = 0; i<len; i++) {
			thisc = string[i];
			if(lastc == ' ' && thisc == ' ' && i < len - 1) {
				j++;
				maxchoice++;
			}
			if(j == *choice && i < len - 1) {
				fstr[i] = c_hitext;
				bstr[i] = c_hiback;
			}
			else {
				fstr[i] = c_text;
				bstr[i] = c_back;
			}
			if(isupper(thisc)) {
				fstr[i] = c_hiletter; // hilite first capital letter.
				if(j == *choice) cpos = i;
			}
			lastc = thisc;
		}

		// Menu bar starting position.
		vga4bit.textxy(col, row);

		// put the attributes then characters to video. 
		for(i = 0; i < len; i++) {
          if(isupper(string[i])) {
		    vga4bit.setForegroundColor(c_hiletter);
		  } else {
            vga4bit.setForegroundColor(fstr[i] & 0x0f);
          }
          vga4bit.setBackgroundColor(bstr[i] & 0x0f);
          vga4bit.printf("%c",string[i]); // Print the next char of the string.
		}

		// put cursor at appropriate position
        vga4bit.textxy((col + cpos), row);
		
		key = toupper(getkey_or_mouse()); // Un-comment this to use the keyboard and mouse.
//		key = toupper(USBGetkey());  // Un-comment this to use just the keyboard.

		/* check for alpha key */
		if(isalpha((unsigned char)key)) {
			for (i = 0; i < len; i++) {
				if(++cpos >= len) {
					cpos = 0;
					*choice = 0;
				}
				if(isupper(string[cpos]))
					*choice += 1;
				if(string[cpos] == (char)key)
					break;
			}
		}
		
		// check for control keys
		switch(key) {
			case	MY_KEY_LEFT:
				if(*choice > 1)
					*choice -= 1;
				break;
			case	MY_KEY_RIGHT:
				if(*choice < maxchoice)
					*choice += 1;
				break;
			case	MY_KEY_HOME:
				*choice = 1;
				break;
			case	MY_KEY_END:
				*choice = maxchoice;
				break;
			case	MY_KEY_UP:
				*choice = 0;
				quit_flag = 1;
				break;
			case	MY_KEY_ENTER:
			case	MY_KEY_DOWN:
				quit_flag = 1;
				break;
		}
	}
    vga4bit.setForegroundColor(fColor);
    vga4bit.setBackgroundColor(bColor);
    vga4bit.textxy(curx,cury);
	return(savebuf);
}

uint8_t *menu_drop(int row, int col, const char **strary, int *choice)
{

	int	n = 0;
	uint8_t	len = 0;
	int	tmpcol;
	int	maxchoice;
	int	i;
	int	quit_flag = 0;
    uint8_t *savebuf;
	char	key;
    const char *firstLetter = NULL;

    uint8_t fColor = vga4bit.getTextFGC();  
    uint8_t bColor = vga4bit.getTextBGC();  
    uint8_t curx = vga4bit.getTextX();
    uint8_t cury = vga4bit.getTextY();

	while(strary[n] != NULL)
		n++;
		
	maxchoice = n - 2;
	
	// Find longest string in string array.
	for (i = 0; i < n; i++)
		if(strlen(strary[i]) > len)
			len = strlen(strary[i]);
	
	/* Save the menu background */
	if(mb_shadow) {
		savebuf = (uint8_t *)vbox_get(row, col, row + n, col + len + 5);
	} else {
		savebuf = (uint8_t *)vbox_get(row, col, row + n - 1, col + len + 3);
    }

    // Create the menu box.
    vga4bit.setForegroundColor(c_lines);
    vga4bit.setBackgroundColor(c_back);
	box_erase(row, col, row + n - 1, col + len + 3);
	box_draw(row, col, row + n - 1, col + len + 3, mb_lines);
	
	/* Cast a shadow */
	if(mb_shadow) {
        vga4bit.setForegroundColor(VGA_GREY);
        vga4bit.setBackgroundColor(VGA_BLACK);
		box_charfill(row + n, col + 2, row + n, col + len + 3, B_PATTERN);
		box_charfill(row + 1, col + len + 4, row + n, col + len + 5, B_PATTERN);
	}

    // Print sub menu Title.
	tmpcol = col + (len - strlen(strary[0]) + 4) / 2;
    vga4bit.textxy(tmpcol, row);
	vga4bit.setForegroundColor(c_title);
    vga4bit.setBackgroundColor(c_back);
	vga4bit.printf("%s",strary[0]);

    // print menu entries.
	vga4bit.setForegroundColor(c_text);
	for (i = 1; i <= maxchoice; i++) {
        vga4bit.textxy(col + 2, (row + i));
		vga4bit.printf("%s",strary[i]);
	}
	
	// Print prompt.
	tmpcol = col + (len - strlen(strary[n - 1]) + 4) / 2;
    vga4bit.textxy(tmpcol, (row + n) - 1);
	vga4bit.setForegroundColor(c_prompt);
	vga4bit.printf("%s",strary[n - 1]);
	
	*choice = 1;

	while(!quit_flag) {
		for (i = 1; i <= maxchoice; i++) {
			if (i == *choice) { // Choice matches menu item. (hilite)
				firstLetter = strary[*choice];
				vga4bit.setBackgroundColor(c_hiback);
                vga4bit.setForegroundColor(c_hiback);
				box_color(row + i, col + 1, row + i, col + len + 2);
                vga4bit.setForegroundColor(c_hitext);
                vga4bit.textxy(col + 2,row + i);
				vga4bit.printf("%s",strary[i]);
				vga4bit.setForegroundColor(c_hiletter);
                vga4bit.textxy(col + 2,row + i);
				vga4bit.printf("%c",firstLetter[0]);
			} else { // Choice does not match menu item. (un-hilite)
				firstLetter = strary[i];
                vga4bit.setForegroundColor(c_back);
   				box_color(row + i, col + 1, row + i, col + len + 2);
				vga4bit.setBackgroundColor(c_back);
                vga4bit.setForegroundColor(c_text);
                vga4bit.textxy(col + 2,row + i);
				vga4bit.printf("%s",strary[i]);
				vga4bit.setForegroundColor(c_hiletter);
                vga4bit.textxy(col + 2,row + i);
				vga4bit.printf("%c",firstLetter[0]);
			}
		}
        vga4bit.textxy((col + 2)-1, (row + *choice)-1); // Re-position print position.
		key = toupper(getkey_or_mouse()); // Un-comment this to use the keyboard and mouse.
//		key = toupper(USBGetkey()); // Un-comment this to use just the keyboard.

		/* check for alpha key */
		if(isalpha((unsigned char)key)) {
			for (i = 1; i <= maxchoice; i++) {
				*choice += 1;
				
				if(*choice > maxchoice)
					*choice = 1;
				if(strary[*choice][0] == (char)key)
					break;
			}
		}
		/* check for control keys */
		switch(key) {
			case	MY_KEY_UP:
				if(*choice > 1)
					*choice -= 1;
				break;
			case	MY_KEY_DOWN:
				if(*choice < maxchoice)
					*choice += 1;
				break;
			case	MY_KEY_HOME:
				*choice = 1;
				break;
			case	MY_KEY_END:
				*choice = maxchoice;
				break;
			case	MY_KEY_ESCAPE:
				*choice = 0;
				quit_flag = 1;
				break;
			case	MY_KEY_ENTER:
				quit_flag = 1;
				break;
		}
	}
    vga4bit.setForegroundColor(fColor);
    vga4bit.setBackgroundColor(bColor);
    vga4bit.textxy(curx,cury);
	return(savebuf);
}

// Create a messsage box. (see mtext.h for message format)
uint8_t *menu_message(int row, int col, const char **strary) {
	
	int	n = 0;
	uint8_t	len = 0;
	int	tmpcol;
	int	i;
    uint8_t *savebuf;

    uint8_t fColor = vga4bit.getTextFGC();  
    uint8_t bColor = vga4bit.getTextBGC();  
    uint8_t curx = vga4bit.getTextX();
    uint8_t cury = vga4bit.getTextY();

	// Get strary[] length.
	while(strary[n] != NULL)
		n++;
	
	// Find longest string in string array.
	for (i = 0; i < n; i++)
		if(strlen(strary[i]) > len)
			len = strlen(strary[i]);
	
	/* Save the message background */
	if(mb_shadow) // Allocate and save section of screen including shadow if enabled.
		savebuf = (uint8_t *)vbox_get(row, col, row + n, col + len + 5);
	else // Ditto: without shadow.
		savebuf = (uint8_t *)vbox_get(row, col, row + n - 1, col + len + 3);

	/* Create the information box */
    vga4bit.setForegroundColor(c_lines);
    vga4bit.setBackgroundColor(c_back);
	box_erase(row, col, row + n - 1, col + len + 3);
	box_draw(row, col, row + n - 1, col + len + 3, mb_lines);

	/* Cast a shadow */
	if(mb_shadow) {
      vga4bit.setForegroundColor(VGA_GREY);
      vga4bit.setBackgroundColor(VGA_BLACK);
	  box_charfill(row + n, col + 2, row + n, col + len + 3, B_PATTERN);
	  box_charfill(row + 1, col + len + 4, row + n, col + len + 5, B_PATTERN);
	}

	// Display title.
	tmpcol = col + (len - strlen(strary[0]) + 4) / 2;
	vga4bit.textxy(tmpcol-1, row);
    vga4bit.setForegroundColor(c_title);
    vga4bit.setBackgroundColor(c_back);
	vga4bit.printf("%s\n",strary[0]);

    // Display message.
    vga4bit.setForegroundColor(c_text);
	for (i = 1; i < n - 1; i++) {
		vga4bit.textxy((col + 2)-1, (row + i));
		vga4bit.printf("%s\n",strary[i]);
	}
	
	// Display prompt.
	tmpcol = col + (len - strlen(strary[n - 1]) + 4) / 2;
    vga4bit.textxy(tmpcol-1, row + n - 1);
    vga4bit.setForegroundColor(c_prompt);
	vga4bit.printf("%s\n",(strary[n - 1]));

    vga4bit.setForegroundColor(fColor);
    vga4bit.setBackgroundColor(bColor);
    vga4bit.textxy(curx,cury);
		
	return((uint8_t *)savebuf); // Return pointer to buffer.

}

// Restore saved screen box and free buffer.
void menu_erase(uint8_t *buf) {
	vbox_put(buf);
	free(buf);
}
