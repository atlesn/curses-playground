#include <stdio.h>
#include <stdlib.h>

#include "win.h"

#define WIN_COUNT 16
struct win windows[WIN_COUNT] = {0};

static void log_update (void) {
	char tmp[256];
	sprintf(tmp, "%i", rand());
	win_text_push(&windows[0], tmp);
}

void main_init (void) {
	win_init(&windows[0], "Log", 20, 0, BOTTOM, 0);
	win_init(&windows[1], "Sidebar", 0, 30, 0, RIGHT);
	win_init(&windows[2], "Main", 0, 0, 0, 0);
	win_init(&windows[3], "Dialogue", 5, 30, 0, 0);

	text_push(&windows[3].text, "Press s/l to show windows and S/L to hide windows");
	text_push(&windows[3].text, "Press tab or shift+tab to change focus");
	text_push(&windows[3].text, "Press q to quit");
	win_fit_text(&windows[3]);

	// Sidebar takes remaining rows after Log
	win_set_post_update_notify_action(&windows[0], &windows[1], WIN_UPDATE_NOTIFY_ACTION_TAKE_REST, 0);

	// Rows of Main set to rows of Sidebar and cols of Main set to remaining cols after Sidebar
	win_set_post_update_notify_action(&windows[1], &windows[2], WIN_UPDATE_NOTIFY_ACTION_COPY, WIN_UPDATE_NOTIFY_ACTION_TAKE_REST);

	// Dialogue put inside Main
	win_set_post_update_notify_action(&windows[2], &windows[3], WIN_UPDATE_NOTIFY_ACTION_PUT_INSIDE, WIN_UPDATE_NOTIFY_ACTION_PUT_INSIDE);

	// Dialogue fit text
	win_set_post_update_action(&windows[3], WIN_UPDATE_ACTION_FIT_TEXT);

	win_show(&windows[2]);
	win_show(&windows[3]);
}

void main_key (int *do_quit, int *focus_change, int *need_update, int key1, int key2, int key3) {
	switch (key3) {
		case 'l':
			win_show(&windows[0]);
			*need_update = 1;
			break;
		case 'L':
			win_hide(&windows[0]);
			*need_update = 1;
			break;
		case 's':
			win_show(&windows[1]);
			*need_update = 1;
			break;
		case 'S':
			win_hide(&windows[1]);
			*need_update = 1;
			break;
		case 'q':
			*do_quit = 1;
			break;
		default:
			break;
	}

	log_update();
}

int main (int argc, const char **argv) {
	win_run (
		windows,
		WIN_COUNT,
		main_init,
		main_key
	);
}
