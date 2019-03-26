#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include "display.h"

#define CLEAR_STRING "\033[2J"

#define TRUE 1
#define FALSE 0

static void windowRender(window_t *window);
static void yankWindow(window_t *window);
static char ** buildStringBlock(unsigned int rows, unsigned int cols);
static color_t ** buildColorBlock(unsigned int rows, unsigned int cols, color_t color);
static void freeDisplayContent(display_t* display);

/**
 * This function builds a new display space.
 * The display space can be used to hold windows that contain content.
 * The display will render to a preset size given by the rows and cols values.
 * The output will be displayed on the terminal passed in.
 *
 * @param term the terminal to display on
 * @param rows the number of rows to display
 * @param cols the number of columns to display
 * @return the new display object
 */
display_t *newDisplay(FILE *term,
				int rows,
				int cols)
{
	display_t *display = (display_t *)malloc(sizeof(display_t));
	display->term = term;
	display->current = buildStringBlock(rows, cols);
	#ifdef DISPLAY_COLOR
	display->current_color = buildColorBlock(rows, cols, WHITE);
	display->default_color = WHITE;
	#endif
	#ifdef DISPLAY_BACKGROUND
	display->current_background = (background_t **) buildColorBlock(rows, cols, -1);
	display->default_background = BLACK;
	#endif
	display->dim.x = rows;
	display->dim.y = cols;
	display->top_window = NULL;
	display->hidden = FALSE;
	display->bottom_window = NULL;
	fprintf(display->term, CLEAR_STRING);
	fprintf(display->term, "\033[%d;0H", rows + 1);
	fprintf(display->term, "\033[?25l");
	fflush(display->term);
	display->dirty = TRUE;
	display->auto_size = FALSE;
	return display;
}

/**
 * This function creates a new window on the given display space.
 * Windows are a child of a display space and will be rendered to their given
 * display space.
 * The window can be bordered or non bordered and is used to display text.
 * 
 * All new windows will be placed on top of the window stack
 *
 * @param display the display the window will be on
 * @param boarder set the boarder on or off
 * @param pos_x the row of the window's content (not boarder) on the display
 * @param pos_y the column of the window's content on the display
 * @param dim_x the number of content rows (not including boarder)
 * @param dim_y the number of content columns (not including boarder)
 * @return the new window
 */
window_t *newWindow(display_t *display,
				char boarder,	
				int pos_x, 
				int pos_y, 
				int dim_x,
				int dim_y)
{
	if (display == NULL)
		return NULL;
	int px = boarder ? pos_x - 1: pos_x;
	int py = boarder ? pos_y - 1: pos_y;
	int dx = boarder ? dim_x + 2: dim_x;
	int dy = boarder ? dim_y + 2: dim_y;
	window_t *window = (window_t *)malloc(sizeof(window_t));
	window->contents = buildStringBlock(dx, dy);
	#ifdef DISPLAY_COLOR
	window->colors = buildColorBlock(dx, dy, WHITE);
	#endif
	#ifdef DISPLAY_BACKGROUND
	window->backgrounds = (background_t **) buildColorBlock(dx, dy, BLACK);
	#endif
	window->pos.x = px;
	window->pos.y = py;
	window->dim.x = dx;
	window->dim.y = dy;
	#ifdef DISPLAY_COLOR
	window->color = RESET;
	#endif
	#ifdef DISPLAY_BACKGROUND
	window->background = BLACK;
	window->setBack = FALSE;
	#endif
	window->hidden = FALSE;
	window->display = display;
	window->boarder = boarder;
	window->next = NULL;

	if (window->display->top_window == NULL)
	{
		window->last = NULL;
		display->bottom_window = window;
	}
	else
	{
		window->last = display->top_window;
		display->top_window->next = window;
	}
	display->top_window = window;


	if (boarder)
	{
		window->contents[0][0] = '+';
		window->contents[0][dy-1] = '+';
		window->contents[dx-1][0] = '+';
		window->contents[dx-1][dy-1] = '+';

		int i;
		for (i = 1; i < dx - 1; i++)
		{
			window->contents[i][0] = '|';
			window->contents[i][dy-1] = '|';
		}
		for (i = 1; i < dy - 1; i++)
		{
			window->contents[0][i] = '-';
			window->contents[dx-1][i] = '-';
		}
	}

	window->display->dirty = TRUE;
	return window;
}

static char isNotPrinted(char c)
{
	if (c == '\n') return FALSE;
	if (c == '\r') return FALSE;
	return TRUE;
}

/**
 * This function prints a string onto a window at a given start location.
 * If a string over flows, it will clip onto the next line.
 *
 * @param window the window being printed to
 * @param str the string being printed
 * @param x the start row
 * @param y the start column
 */
void windowPrint(window_t *window,
				char * str,
				int x,
				int y)
{
	int i;
	int d = window->boarder ? 1: 0;
	for (i = 0; str[i] != '\0'; i++)
	{
	 	if (x + d >= window->dim.x - d)
	 		return;
	 	if (isNotPrinted(str[i]) && i + y + d < window->dim.y - d)
	 	{
	 		if (str[i] == '\t')	
	 		{
	 			int pos = i + y;
	 			int end = ((pos + 8) / 8) * 8;
	 			for (; i + y < end; y++) {
	 				window->contents[x + d][i + y + d] = ' ';
			 		#ifdef DISPLAY_COLOR
					window->colors[x + d][i + y + d] = window->color;
					#endif
					#ifdef DISPLAY_BACKGROUND
					if (window->setBack)
							window->backgrounds[x + d][i + y + d] = window->background;
					#endif
	 			}
	 		}
	 		window->contents[x + d][i + y + d] = str[i];
	 		#ifdef DISPLAY_COLOR
			window->colors[x + d][i + y + d] = window->color;
			#endif
			#ifdef DISPLAY_BACKGROUND
			if (window->setBack)
					window->backgrounds[x + d][i + y + d] = window->background;
			#endif
	 	}
	 	else
	 	{
	 		if (isNotPrinted(str[i]))
	 			i--;
	 		if (str[i] == '\r')
	 		{
	 			if (str[i + 1] != '\n')
	 				x++;
	 		} else {
	 			x++;
	 			y = -1 - i;
	 		}
	 	}
	}
	window->display->dirty = TRUE;
}

/**
 * This function prints a string just like the windowPrint function.
 * However this function has the option for color.
 *
 * @param window the window being printed to
 * @param str the string being printed
 * @param color the color being used
 * @param x the start row
 * @param y the start column
 */
void windowPrintColor(window_t *window,
				char *str,
				color_t color,
				int x,
				int y)
{
	#ifdef DISPLAY_COLOR
	color_t orginal = window->color;
	window->color = color;
	#endif
	windowPrint(window, str, x, y);
	#ifdef DISPLAY_COLOR
	window->color = orginal;
	#endif
}

void windowPrintBackground(window_t * window,
				char *str,
				color_t color,
				background_t background,
				int x,
				int y)
{
	#ifdef DISPLAY_BACKGROUND
	window->setBack = TRUE;
	background_t original = window->background;
	window->background = background;
	#endif
	windowPrintColor(window, str, color, x, y);
	#ifdef DISPLAY_BACKGROUND
	window->background = original;
	window->setBack = FALSE;
	#endif
}

/**
 * This function prints a single char to a given window.
 * If the char is out of bounds, nothing will be changed.
 *
 * @param window the window being used
 * @param c the char being displayed
 * @param x the row
 * @param y the column
 */
void windowChar(window_t *window,
				char c,
				int x,
				int y)
{
	window->display->dirty = TRUE;
	int d = window->boarder ? 1: 0;
    x = x + d;
    y = y + d;
    if (x > 0 && x < window->display->dim.x - d && y > 0 && y < window->display->dim.y)
	{
    	window->contents[x][y] = c;
    	#ifdef DISPLAY_COLOR
		window->colors[x][y] = window->color;
		#endif
		#ifdef DISPLAY_BACKGROUND
		window->backgrounds[x][y] = window->background;
		#endif
	}
}

/**
 * The is function prints a single char in a given color
 *
 * @param window the window being used
 * @param c the char being displayed
 * @param color the color being used
 * @param x the row
 * @param y the column
 */
void windowCharColor(window_t *window,
				char c,
				color_t color,
				int x,
				int y)
{
	#ifdef DISPLAY_COLOR
	color_t original = window->color;
	window->color = color;
	#endif
	windowChar(window, c, x, y);
	#ifdef DISPLAY_COLOR
	window->color = original;
	#endif
}

void windowSetColor(window_t *window, color_t color)
{
	#ifdef DISPLAY_COLOR
	color_t original = window->color;
	int d = window->boarder ? 1 : 0;
	int i, j;
	for (i = d; i < window->dim.x - d; i++)
	{
		for (j = d; j < window->dim.y - d; j++)
		{
			if (window->colors[i][j] == original)
			{
				window->colors[i][j] = color;
			}
		}
	}
	window->color = color;
	window->display->dirty = TRUE;
	#endif
}

void windowSetBackground(window_t *window, background_t background)
{
	#ifdef DISPLAY_BACKGROUND
	background_t original = window->background;
	int d = window->boarder ? 1 : 0;
	int i, j;
	for (i = d; i < window->dim.x - d; i++)
	{
		for (j = d; j < window->dim.y - d; j++)
		{
			if (window->backgrounds[i][j] == original)
			{
				window->backgrounds[i][j] = background;
			}
		}
	}
	window->background = background;
	window->display->dirty = TRUE;
	#endif
}

void windowDrawBackground(window_t *window,
				background_t background,
				int start_x,
				int start_y,
				int end_x,
				int end_y)
{
	#ifdef DISPLAY_BACKGROUND
	int d = window->boarder ? 1: 0;
	int i, j;
	for (i = start_x + d; i < end_x + d && i < window->dim.x - d; i++)
	{
		for (j = start_y + d; j < end_y + d && i < window->dim.y - d; j++)
		{
			window->backgrounds[i][j] = background;
		}
	}
	window->display->dirty = TRUE;
	#endif
}

void windowSetBoarder(window_t * window,
				color_t color,
				background_t background,
				char vertical,
				char horizontal,
				char corner)
{
	if (!window->boarder)
			return;

	int dx = window->dim.x;
	int dy = window->dim.y;

	window->contents[0][0] = corner;
	window->contents[0][dy-1] = corner;
	window->contents[dx-1][0] = corner;
	window->contents[dx-1][dy-1] = corner;

	int i;
	for (i = 1; i < dx - 1; i++)
	{
		window->contents[i][0] = vertical;
		window->contents[i][dy-1] = vertical;
	}
	for (i = 1; i < dy - 1; i++)
	{
		window->contents[0][i] = horizontal;
		window->contents[dx-1][i] = horizontal;
	}
	windowColorBoarder(window, color, background);
}

void windowColorBoarder(window_t * window,
				color_t color,
				background_t background)
{
	if (!window->boarder)
			return;
	int i;
	for (i = 0; i < window->dim.x; i++)
	{
		#ifdef DISPLAY_COLOR
		window->colors[i][0] = color;
		window->colors[i][window->dim.y - 1] = color;
		#endif
		#ifdef DISPLAY_BACKGROUND
		window->backgrounds[i][0] = background;
		window->backgrounds[i][window->dim.y - 1] = background;
		#endif
	}
	for (i = 0; i < window->dim.y; i++)
	{
		#ifdef DISPLAY_COLOR
		window->colors[0][i] = color;
		window->colors[window->dim.x - 1][i] = color;
		#endif
		#ifdef DISPLAY_BACKGROUND
		window->backgrounds[0][i] = background;
		window->backgrounds[window->dim.x - 1][i] = background;
		#endif
	}
	window->display->dirty = TRUE;
}

/**
 * This function clears the content of a window
 *
 * @param window the window to be cleared
 */
void windowClear(window_t *window)
{
	int d = window->boarder ? 1: 0;
	int i, j;
	for (i = d; i < window->dim.x - d; i++)
	{
		for (j = d; j < window->dim.y - d; j++)
		{
			window->contents[i][j] = '\0';
			#ifdef DISPLAY_COLOR
			window->colors[i][j] = window->color;
			#endif
			#ifdef DISPLAY_BACKGROUND
			window->backgrounds[i][j] = window->background;
			#endif
		}
	}
	window->display->dirty = TRUE;
}

void windowSetHide(window_t *window, char hidden)
{
	window->hidden = hidden;
	window->display = TRUE;
}

void displaySetHide(display_t *display, char hidden)
{
	if (hidden && !display->hidden)
	{
		int i, j;
		for (i = 0; i < display->dim.x; i++)
		{
			for (j = 0; j < display->dim.y; j++)
				{
					display->current[i][j] = '\0';
					#ifdef DISPLAY_COLOR
					display->current_color[i][j] = WHITE;
					#endif
					#ifdef DISPLAY_BACKGROUND
					display->current_background[i][j] = -1;
					#endif
				}
		}
		fprintf(display->term, CLEAR_STRING);
		fprintf(display->term, "\033[1;1H");
		fprintf(display->term, "\033[?25h");
		fprintf(display->term, "\1");
		fflush(display->term);
	}
	else if (!hidden && display->hidden)
	{
		fprintf(display->term, CLEAR_STRING);
		//fprintf(display->term, "\033[%d;0H", display->dim.x + 1);
		fprintf(display->term, "\033[?25l");
		//fprintf(display->term, "\1");
		//fflush(display->term);
		display->dirty = TRUE;
	}
	display->hidden = hidden;
}


/**
 * This function sets the given window to the top of the window stack
 *
 * @param window the window being put on top
 */
void setTopWindow(window_t *window)
{
	if (window->display->top_window == window)
		return;
	yankWindow(window);
	window->next = NULL;
	window->last = window->display->top_window;
	window->display->top_window->next = window;
	window->display->top_window = window;
	window->display->dirty = TRUE;
}

/**
 * This function deletes a window
 *
 * @param window the window to be deleted
 */
void freeWindow(window_t *window)
{
	yankWindow(window);
	int i = 0;
	for (i = 0; i < window->dim.x; i++)
	{
		free(window->contents[i]);
		#ifdef DISPLAY_COLOR
		free(window->colors[i]);
		#endif
		#ifdef DISPLAY_BACKGROUND
		free(window->backgrounds[i]);
		#endif
	}
	free(window->contents);
	#ifdef DISPLAY_COLOR
	free(window->colors);
	#endif
	#ifdef DISPLAY_BACKGROUND
	free(window->backgrounds);
	#endif
	window->display->dirty = TRUE;
	free(window);
}

/**
 * This function deletes a display space and all of its windows
 *
 * @param display the display to be removed
 */
void freeDisplay(display_t *display)
{
	window_t *current, *next;
	for (current = display->bottom_window; current != NULL; current = next)
	{
		next = current->next;
		freeWindow(current);
	}

	freeDisplayContent(display);

	fprintf(display->term, CLEAR_STRING);
	fprintf(display->term, "\033[1;1H");
	fprintf(display->term, "\033[?25h");

	free(display);
}

static struct char_struct {
	char data;
	#ifdef DISPLAY_COLOR
	color_t color;
	#endif
	#ifdef DISPLAY_BACKGROUND
	background_t background;
	#endif
};

static struct char_struct renderPoint(display_t* display, window_t * window, int x, int y)
{
	struct char_struct char_data;
	char_data.data = ' ';
	#ifdef DISPLAY_COLOR
	char_data.color = display->default_color;
	#endif
	#ifdef DISPLAY_BACKGROUND
	char_data.background = display->default_background;
	#endif

	if (window == NULL) return char_data;

	window_t *current;
	for (current = window; current != NULL; current = current->next)
	{
		if(!window->hidden)
		{
			int i = x - current->pos.x;
			if (x >= 0 && x < current->display->dim.x && i >= 0 && i < current->dim.x)
			{
				int j = y - current->pos.y;
				if (y >= 0 && y < current->display->dim.y && j >= 0 && j < current->dim.y)
				{
					char_data.data = current->contents[i][j];
					#ifdef DISPLAY_COLOR
					char_data.color = current->colors[i][j];
					#endif
					#ifdef DISPLAY_BACKGROUND
					char_data.background = current->backgrounds[i][j];
					#endif
				}
				
			}
		}
	}

	return char_data;
}

/**
 * This function pulls a window out of the display stack
 *
 * @param window the window to be unlinked
 */
void yankWindow(window_t * window)
{
	if (window->display->top_window == window)
	{
		window->display->top_window = window->last;
		if (window->last == NULL)
		{
			window->display->bottom_window = NULL;
			return;
		}
		window->last->next = NULL;
		return;
	}
	window->next->last = window->last;
	if (window->last != NULL)
	{
		window->last->next = window->next;
	}
	else
	{
		window->display->bottom_window = window->next;
	}
}

/**
 * this function builds a 2D char ** array and sets all the values to null
 *
 * @param x the x size
 * @param y the y size
 * @return the 2D char array
 */
char ** buildStringBlock(unsigned int x, unsigned int y)
{
	char ** data = (char **)malloc(sizeof(char *) * x);
	int i, j;
	for (i = 0; i < x; i++)
	{
		data[i] = (char*)malloc(sizeof(char) * (y + 1));
		for (j = 0; j <= y; j++)
		{
			data[i][j] = '\0';
		}
	}
	return data;
}

color_t ** buildColorBlock(unsigned int x, unsigned int y, color_t color)
{
	color_t ** data = (color_t **)malloc(sizeof(color_t *) * x);
	int i, j;
	for (i = 0; i < x; i++)
	{
		data[i] = (color_t *)malloc(sizeof(color_t) * (y + 1));
		for (j = 0; j <= y; j++)
		{
			data[i][j] = color;
		}
	}
	return data;
}

static void freeDisplayContent(display_t* display)
{
	int i;
	for (i = 0; i < display->dim.x; i++)
	{
		free(display->current[i]);
		#ifdef DISPLAY_COLOR
		free(display->current_color[i]);
		#endif
		#ifdef DISPLAY_BACKGROUND
		free(display->current_background[i]);
		#endif
	}
	free(display->current);
	#ifdef DISPLAY_COLOR
	free(display->current_color);
	#endif
	#ifdef DISPLAY_BACKGROUND
	free(display->current_background);
	#endif
}

void displaySetSize(display_t* display, int rows, int cols)
{
	if (rows == display->dim.x && cols == display->dim.y)
		return;

	freeDisplayContent(display);

	display->dim.x = rows;
	display->dim.y = cols;
	display->dirty = TRUE;

	display->current = buildStringBlock(rows, cols);
	#ifdef DISPLAY_COLOR
	display->current_color = buildColorBlock(rows, cols, display->default_color);
	#endif
	#ifdef DISPLAY_BACKGROUND
	display->current_background = buildColorBlock(rows, cols, display->default_background);
	#endif
}

static void checkAndUpdateDisplaySize(display_t* display)
{
	if (!display->auto_size) return;

	struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);

    displaySetSize(display, w.ws_row, w.ws_col);
}

/**
 * This is the print task for the display.
 * It will only print diffs between the current and next maps.
 * The cursor will keep its position between updates
 *
 * @param display the display data
 */
void displayUpdate(display_t* display)
{
	checkAndUpdateDisplaySize(display);

	if (display->hidden || !display->dirty)
	{
		return;
	}
	display->dirty = FALSE;

	struct char_struct next;
	int current_x = -1;
	int current_y = -1;
	#ifdef DISPLAY_COLOR
	int current_color = -1;
	#endif
	#ifdef DISPLAY_BACKGROUND
	int current_background = -1;
	#endif
	int i, j;
	fprintf(display->term, "\033[s");
	//fprintf(display->term, "\033[?25l");
	for (i = 0; i < display->dim.x; i++)
	{
		for (j = 0; j < display->dim.y; j++)
		{
			next = renderPoint(display, display->bottom_window, i, j);
			if (display->current[i][j] != next.data
				#ifdef DISPLAY_COLOR
				|| display->current_color[i][j] != next.color
				#endif
				#ifdef DISPLAY_BACKGROUND
				|| display->current_background[i][j] != next.background
				#endif
				)
			{
				if (current_x != i || current_y != j)
				{
					fprintf(display->term, "\033[%d;%dH", i + 1, j + 1);
					current_x = i;
					current_y = j;
				}
				
				#ifdef DISPLAY_COLOR
				if (current_color != next.color)
				{
					fprintf(display->term, "\033[38;5;%dm", next.color);
					current_color = next.color;
				}
				#endif
				#ifdef DISPLAY_BACKGROUND
				if (current_background != next.background)
				{
					fprintf(display->term, "\033[48;5;%dm", next.background);
					current_background = next.background;
				}
				#endif

				char c = next.data;
				if (isspace((int)c) || c == '\0')
				{
					next.data = ' ';
				}
				fprintf(display->term, "%c", next.data);
				display->current[i][j] = next.data;
				#ifdef DISPLAY_COLOR
				display->current_color[i][j] = next.color;
				#endif
				#ifdef DISPLAY_BACKGROUND
				display->current_background[i][j] = next.background;
				#endif
				current_y++;
			}
		}
	}
	#ifdef DISPLAY_COLOR
	if (current_color != RESET)
			fprintf(display->term, "\033[0m");
	#endif
	fprintf(display->term, "\033[u");
	//fprintf(display->term, "\033[?25h");
	fflush(display->term);
}

void displaySetAutoSize(display_t* display, char autoSet)
{
	display->auto_size = autoSet;
}