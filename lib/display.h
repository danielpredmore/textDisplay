#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>

#define DISPLAY_BACKGROUND
#define DISPLAY_COLOR

/**
 * display.h is a lite display driver
 * with window support
 *
 *
 * +--------------------+
 * | This is a test     |
 * | window.            |
 * |                    |
 * +--------------------+
 */

enum color_enum
{
	RESET = 7,
	BLACK = 0,
	RED = 1,
	GREEN = 2,
	YELLOW = 3,
	BLUE = 4,
	MAGENTA = 5,
	CYAN = 6,
	WHITE = 7,
	GREAY = 8,
	BRIGHT_RED = 9,
	BRIGHT_GREEN = 10,
	BRIGHT_YELLOW = 11,
	BRIGHT_BLUE = 12,
	BRIGHT_MAGENTA = 13,
	BRIGHT_CYAN = 14,
	BRIGHT_WHITE = 15
};

typedef unsigned char color_t;
typedef unsigned char background_t;

struct window_struct;

struct point_struct
{
	int x;
	int y;
};
typedef struct point_struct point_t;
typedef struct point_struct dimension_t;

struct display_struct
{
	char ** current;
	#ifdef DISPLAY_COLOR
	color_t ** current_color;
	color_t default_color;
	#endif
	#ifdef DISPLAY_BACKGROUND
	background_t ** current_background;
	color_t default_background;
	#endif
	dimension_t dim;
	FILE *term;
	struct window_struct *top_window;
	struct window_struct *bottom_window;
	char hidden;
	char dirty;
};
typedef struct display_struct display_t;

struct window_struct
{
	char ** contents;
	#ifdef DISPLAY_COLOR
	color_t color;
	color_t ** colors;
	#endif
	#ifdef DISPLAY_BACKGROUND
	background_t background;
	background_t ** backgrounds;
	#endif
	point_t pos;
	dimension_t dim;
	display_t *display;
	char boarder;
	#ifdef DISPLAY_BACKGROUND
	char setBack;
	#endif
	char hidden;
	struct window_struct *next;
	struct window_struct *last;
};
typedef struct window_struct window_t;

display_t *newDisplay(FILE *term, 
				int rows, 
				int cols);
window_t *newWindow(display_t *display,
			    char boarder,	
				int pos_x, 
				int pos_y, 
				int dim_x,
				int dim_y);
void windowPrint(window_t *window,
				char *str,
				int x,
				int y);
void windowPrintColor(window_t *window,
				char *str,
				color_t color,
				int x,
				int y);
void windowPrintBackground(window_t *window,
				char *str,
				color_t color,
				background_t background,
				int x,
				int y);
void windowChar(window_t *window,
				char c,
				int x,
				int y);
void windowCharColor(window_t *window,
				char c,
				color_t color,
				int x,
				int y);
void windowSetColor(window_t *window, color_t color);
void windowSetBackground(window_t *window, background_t background);
void windowDrawBackground(window_t *window,
				background_t background,
				int start_x,
				int start_y,
				int end_x,
				int end_y);
void windowSetBoarder(window_t * window,
				color_t color,
				background_t background,
				char vertical,
				char horizontal,
				char corner);
void windowColorBoarder(window_t * window,
				color_t color,
				background_t background);
void windowClear(window_t *window);
void windowSetHide(window_t *window, char hidden);
void displaySetHide(display_t *display, char hidden);
void SetTopWindow(window_t *window);
void freeWindow(window_t *window);
void freeDisplay(display_t *display);
void displayUpdate(display_t* display);

#endif // DISPLAY_H
