#ifndef MTEXT_H
#define MTEXT_H

const char	*info_box[] =
			{
			" INSTRUCTIONS ",
			"This is a demonstration of the MENU functions.  The menu bar",
			"above lets you select and try the many combinations of",
			"menu display options.  To access a menu, press the cursor",
			"movment keys or the first letter of a choice. The enter key",
			"selects the hilighted item.  The ESC key cancels a menu.",
			"< This box was created by menu_message() >",
			(const char *)'\0'
			};

const char	*bar_main = "  Lines  Colors  Shadow  Quit  ";

const char	*drop_quit[] = {
			"< Quit? >",
			"No",
			"Yes",
			"< Select >",
			(const char *)'\0'
			};
	
const char	*drop_lines[] = {
			"< Box Outline >",
			"Single line",
			"Double line",
			"No lines",
			"< Select >",
			(const char *)'\0'
			};

const char *drop_shadow[] = {
	" Menu Shadow Control ",
	"Shadow off ",
	"Shadow on ",
	"< Select >",
	(const char *)'\0'
};	 

const char *drop_colors[] = {
	" Colors ",
	"Background",
	"Border Lines",
	"Titles",
	"Choices text",
	"Prompt",
	"Hilighted letter",
	"Hilighted text",
	"Hilighted background",
	" Up Dn Ltr Enter Esc ",
	(const char *)'\0'
};

const char *drop_t_colors[] = {
	"", /* no title */
	"Black",
	"Blue",
	"Green",
	"Cyan",
	"Red",
	"Magenta",
	"Yellow",
	"White",
	"Grey",
	"Bright blue",
	"Bright green",
	"Bright cyan",
	"Bright red",
	"Bright magenta",
	"Bright yellow",
	"Bright white",
	"", /* No prompt */
	NULL
	};

const char *drop_bk_colors[] = {
	"",
	"Black",
	"Blue",
	"Green",
	"Cyan",
	"Red",
	"Magenta",
	"Brown",
	"White",
	"",
	(const char *)'\0'
	};


#endif
