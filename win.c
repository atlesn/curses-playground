#include <string.h>
#include <stdlib.h>
#include <locale.h>

#define NCURSES_WIDECHAR 1
#include <curses.h>

#include "win.h"

void win_fit_text(struct win *win) {
	const int longest = text_get_longest(&win->text);
	if (longest > win->cols - 4 && longest - 4 > win->cols) {
		win->cols = longest + 4;
	}
}

void win_set_post_update_notify_action (
		struct win *target,
		struct win *subject,
		int row_action,
		int col_action
) {
	target->win_post_update_notify_subject = subject;
	target->post_update_notify_actions_row = row_action;
	target->post_update_notify_actions_col = col_action;
}

void win_set_post_update_action (
		struct win *subject,
		int actions
) {
	subject->post_update_actions = actions;
}

static void win_process_post_update_notify_action (
		struct win *subject,
		const struct win *source
) {
	if (source->post_update_notify_actions_row & WIN_UPDATE_NOTIFY_ACTION_COPY) {
		subject->rows = source->rows;
		subject->rows_command = UPDATE_INHIBIT;
	}
	if (source->post_update_notify_actions_col & WIN_UPDATE_NOTIFY_ACTION_COPY) {
		subject->cols = source->cols;
		subject->cols_command = UPDATE_INHIBIT;
	}
	if (source->post_update_notify_actions_row & WIN_UPDATE_NOTIFY_ACTION_TAKE_REST) {
		if (source->showing) {
			subject->rows = source->showing ? LINES - source->rows : LINES;
			subject->rows_command = UPDATE_INHIBIT;
		}
		else {
			subject->rows = LINES;
		}
	}
	if (source->post_update_notify_actions_col & WIN_UPDATE_NOTIFY_ACTION_TAKE_REST) {
		if (source->showing) {
			subject->cols = source->showing ? COLS - source->cols : COLS;
			subject->cols_command = UPDATE_INHIBIT;
		}
		else {
			subject->cols = LINES;
		}
	}
	if ((source->post_update_notify_actions_col|source->post_update_notify_actions_row) & WIN_UPDATE_NOTIFY_ACTION_PUT_INSIDE) {
		subject->y = source->rows / 2 - subject->rows / 2 + source->y;
		subject->x = source->cols / 2 - subject->cols / 2 + source->x;
		subject->y_command = UPDATE_INHIBIT;
		subject->x_command = UPDATE_INHIBIT;
	}
}

static void win_cleanup (struct win *win) {
	if (win->win)
		delwin(win->win);
	win->win = NULL;
	text_cleanup(&win->text);
}

static void win_reinit (struct win *win) {
	if (win->win)
		delwin(win->win);
	win->win = newwin (
			win->rows,
			win->cols,
			win->y,
			win->x
	);
}

void win_show (struct win *win) {
	if (win->showing)
		return;

	win->showing = 1;
}

void win_hide (struct win *win) {
	if (!win->showing) {
		return;
	}

	win->showing = 0;
}

static int win_yx_resolve_command (int command, int a, int b, int c) {
	int result = command;

	switch (command) {
		case UPDATE_INHIBIT:
			result = c;
			break;
		case CENTER:
			result = a / 2 - b / 2;
			break;
		case BOTTOM:
		case RIGHT:
			result = a - b;
			break;
		deault:
			break;
	};

	return result;
}

static int win_rowscols_resolve_command (int command, int a, int b) {
	int result = command;

	switch (command) {
		case 0:
			result = a;
			break;
		case UPDATE_INHIBIT:
			result = b;
			break;
		default:
			result = command;
			break;
	};

	return result;
}

static void win_update_yx (struct win *win) {
	win->rows = win_rowscols_resolve_command (win->rows_command, LINES, win->rows);
	win->cols = win_rowscols_resolve_command (win->cols_command, COLS, win->cols);

	win->y = win_yx_resolve_command (win->y_command, LINES, win->rows, win->y);
	win->x = win_yx_resolve_command (win->x_command, COLS, win->cols, win->x);

	if (win->rows_command == UPDATE_INHIBIT)
		win->rows_command = 0;
	if (win->cols_command == UPDATE_INHIBIT)
		win->cols_command = 0;
	if (win->y_command == UPDATE_INHIBIT)
		win->y_command = 0;
	if (win->x_command == UPDATE_INHIBIT)
		win->x_command = 0;

	if (win->post_update_actions & WIN_UPDATE_ACTION_FIT_TEXT)
		win_fit_text(win);

	if (win->win_post_update_notify_subject)
		win_process_post_update_notify_action(win->win_post_update_notify_subject, win);
}

static void win_update_inside_text_print_callback (const char *text, const int row, void *callback_arg) {
	struct win *win = callback_arg;

	wmove(win->win, row + 1, 2);
	wprintw(win->win, "%s", text);
}

static void win_update_inside (struct win *win) {
	if (!win->showing || win->rows < 3)
		return;

	text_print(win->cols - 4, win->rows - 2, &win->text, win_update_inside_text_print_callback, win);
	wrefresh(win->win);
}

void win_text_push (struct win *win, const char *text) {
	text_push(&win->text, text);

	if (win->win)
		win_update_inside(win);
}

static int win_draw_border (struct win *win, int focus) {
	cchar_t *v, *h, *tl, *tr, *br, *bl;

	if (focus) {
		v = WACS_T_VLINE;
		h = WACS_T_HLINE;
		tl = WACS_T_ULCORNER;
		tr = WACS_T_URCORNER;
		br = WACS_T_LRCORNER;
		bl = WACS_T_LLCORNER;
	}
	else {
		v = WACS_VLINE;
		h = WACS_HLINE;
		tl = WACS_ULCORNER;
		tr = WACS_URCORNER;
		br = WACS_LRCORNER;
		bl = WACS_LLCORNER;
	}

	// Just make sure window fits
	if (box(win->win, 0, 0) != OK)
		return ERR;

	for (int i = 1; i < win->rows - 1; i++) {
		mvwadd_wch(win->win, i, 0, v);
		mvwadd_wch(win->win, i, win->cols - 1, v);
	}
	for (int i = 1; i < win->cols - 1; i++) {
		mvwadd_wch(win->win, 0, i, h);
		mvwadd_wch(win->win, win->rows - 1, i, h);
	}

	mvwadd_wch(win->win, 0, 0, tl);
	mvwadd_wch(win->win, 0, win->cols - 1, tr);
	mvwadd_wch(win->win, win->rows - 1, win->cols - 1, br);
	mvwadd_wch(win->win, win->rows - 1, 0, bl);

	return OK;
}

static void win_draw (struct win *win, int focus) {
	if (!win->showing)
		return;

	win_reinit(win);

	if (win_draw_border(win, focus) != OK)
		return;
	wrefresh(win->win);

	if (focus)
		wattron(win->win, A_BOLD);

	wmove(win->win, 0, 4) == OK &&
		wprintw(win->win, "  %s %ix%i  ", win->title, win->rows, win->cols) == OK &&
		wrefresh(win->win);
	
	wattroff(win->win, A_BOLD);

	win_update_inside(win);
}

void win_init (struct win *win, const char *title, int rows, int cols, int y, int x) {
	win->title = title;
	win->rows_command = rows;
	win->cols_command = cols;
	win->y_command = y;
	win->x_command = x;
	win_update_yx(win);
}

static void win_update_many (struct win *wins, size_t count, int focus) {
	endwin();
	refresh();

	for (int i = 0; i < count; i++) {
		win_update_yx(&wins[i]);
	}

	for (int i = 0; i < count; i++) {
		win_draw(&wins[i], focus == i);
	}
}

static void win_cleanup_many (struct win *wins, size_t count) {
	for (int i = 0; i < count; i++) {
		win_cleanup(&wins[i]);
	}
}

void win_run (
		struct win *wins,
		const size_t count,
		void (*init_callback)(void),
		void (*key_callback)(int *do_quit, int *focus_change, int *need_update, int key1, int key2, int key3)
) {
	setlocale(LC_ALL, "");

	WINDOW *root = initscr();
	cbreak();
	noecho();
	halfdelay(5);

	init_callback();

	int focus = 0;
	int chbuf[3] = {0};

	win_update_many(wins, count, focus);
	move(3, 0);

	while (1) {
		chbuf[0] = chbuf[1];
		chbuf[1] = chbuf[2];
		chbuf[2] = getch();

		// printf("%i-%i-%i\n", chbuf[0], chbuf[1], chbuf[2]);

		int do_quit = 0;
		int focus_change = 0;
		int need_update = 0;

		if (chbuf[0] == 27 && chbuf[1] == 91 && chbuf[2] == 90) {
			/* SHIFT TAB */
			focus_change = 1;
		}
		else if (chbuf[2] == 9) {
			/* TAB */
			focus_change = -1;
		}
		else if (chbuf[2] == KEY_RESIZE) {
			need_update = 1;
		}
		else {
			key_callback(&do_quit, &focus_change, &need_update, chbuf[0], chbuf[1], chbuf[2]);
		}

		if (do_quit)
			break;

		if (!focus_change && !wins[focus].showing) {
			focus_change = 1;
		}

		if (focus_change) {
			focus += focus_change;
			for (int i = 0; i < count; i++) {
				if (focus == count)
					focus = 0;
				else if (focus < 0)
					focus = count - 1;

				if (wins[focus].showing)
					break;

				focus += focus_change;
			}
			need_update = 1;
		}

		if (need_update) {
			win_update_many(wins, count, focus);
			move(3, 0);
		}
	}

	win_cleanup_many(wins, count);
	endwin();
	exit_curses(0);
}
