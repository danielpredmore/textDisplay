#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <display.h>

#define BUFF_SIZE 80

int main(int argc, char** argv)
{
	display_t* display = newDisplay(stdout, 40, 80);

	window_t* window = newWindow(display, 1, 1, 1, 38, 78);

	displayUpdate(display);

	sleep(3);

	displaySetAutoSize(display, 1);

	displayUpdate(display);

	sleep(3);

	freeWindow(window);

	displayUpdate(display);

	sleep(3);

	freeDisplay(display);

    return 0;
}
