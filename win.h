#ifndef WIN_H
#define WIN_H

#include <stddef.h>

#include "text.h"

#define CENTER -1
#define BOTTOM -2
#define RIGHT -3
#define UPDATE_INHIBIT -4

#define WIN_UPDATE_NOTIFY_ACTION_NONE        (1<<0)
#define WIN_UPDATE_NOTIFY_ACTION_COPY        (1<<1)
#define WIN_UPDATE_NOTIFY_ACTION_TAKE_REST   (1<<2)
#define WIN_UPDATE_NOTIFY_ACTION_PUT_INSIDE  (1<<3)

#define WIN_UPDATE_ACTION_NONE        (0)
#define WIN_UPDATE_ACTION_FIT_TEXT    (1<<1)

typedef struct _win_st WINDOW;

struct win {
	WINDOW *win;
	int showing;
	int rows, cols, y, x;
	int rows_command, cols_command, y_command, x_command;
	const char *title;
	struct text text;
	int post_update_actions;
	int post_update_notify_actions_row, post_update_notify_actions_col;
	struct win *win_post_update_notify_subject;
};

void win_fit_text(struct win *win);
void win_set_post_update_action (
		struct win *subject,
		int actions
);
void win_set_post_update_notify_action (
		struct win *target,
		struct win *subject,
		int row_action,
		int col_action
);
void win_show (struct win *win);
void win_hide (struct win *win);
void win_init (struct win *win, const char *title, int rows, int cols, int y, int x);
void win_text_push (struct win *win, const char *text);
void win_run (
		struct win *wins,
		const size_t count,
		void (*init_callback)(void),
		void (*key_callback)(int *do_quit, int *focus_change, int *need_update, int key1, int key2, int key3)
);

#endif /* WIN_H */
