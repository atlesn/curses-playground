#include <string.h>
#include <stdlib.h>
#include <locale.h>

#define NCURSES_WIDECHAR 1
#include <curses.h>

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

#define TEXT_MAX_LINES 256

struct text {
	char *lines[TEXT_MAX_LINES];
	int latest;
};

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

void text_cleanup(struct text *text) {
	for (int i = 0; i < TEXT_MAX_LINES; i++) {
		if (text->lines[i] != NULL)
			free(text->lines[i]);
	}
	memset(text, '\0', sizeof(*text));
}

void text_push(struct text *text, const char *str) {
	if (text->lines[text->latest])
		text->latest = (text->latest + 1) % TEXT_MAX_LINES;
	if (text->lines[text->latest])
		free(text->lines[text->latest]);
	text->lines[text->latest] = strdup(str);
}

void text_print(struct win *win, struct text *text) {
	if (win->rows < 3 || !text->lines[text->latest])
		return;

	const int line_max = win->cols - 4;
	char tmp[line_max + 1];
	char tmp_blank[line_max + 1];

	memset(tmp_blank, ' ', line_max);
	tmp_blank[line_max] = '\0';

	int pos = text->latest;
	for (int row = win->rows - 2; row > 0; row--) {
		wmove(win->win, row, 2);
		wprintw(win->win, "%s", tmp_blank);
		wmove(win->win, row, 2);
		if (text->lines[pos]) {
			strncpy(tmp, text->lines[pos], line_max);
			tmp[line_max] = '\0';
			wprintw(win->win, "%s", tmp);
			if (--pos < 0)
				pos = TEXT_MAX_LINES - 1;
		}
	}

	wrefresh(win->win);
}

int text_get_longest(struct text *text) {
	int longest = 0;
	for (int i = 0; i < TEXT_MAX_LINES; i++) {
		if (!text->lines[i])
			continue;

		const int length = strlen(text->lines[i]);
		if (length > longest)
			longest = length;
	}

	return longest;
}

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

void win_process_post_update_notify_action (
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

void win_cleanup (struct win *win) {
	if (win->win)
		delwin(win->win);
	win->win = NULL;
	text_cleanup(&win->text);
}

void win_reinit (struct win *win) {
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
/*
	char spaces[COLS];
	memset(spaces, 'a', COLS);
	spaces[win->cols] = '\0';
	for (int i = win->y; i < win->rows + win->y; i++) {
		move (i, win->x);
		wprintw(win->win, "%s", spaces);
	}
*/
}

int win_yx_resolve_command (int command, int a, int b, int c) {
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

int win_rowscols_resolve_command (int command, int a, int b) {
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

void win_update_yx (struct win *win) {
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

void win_update_inside (struct win *win) {
	if (!win->showing)
		return;

	text_print(win, &win->text);
	wrefresh(win->win);
}

void win_draw_border (struct win *win, int focus) {
	int idx = 0;

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
}

void win_draw (struct win *win, int focus) {
	if (!win->showing)
		return;

	win_reinit(win);

	win_draw_border(win, focus);

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

#define WIN_COUNT 16
struct win windows[WIN_COUNT] = {0};

static void update_all (int focus) {
	endwin();
	refresh();

	for (int i = 0; i < WIN_COUNT; i++) {
		win_update_yx(&windows[i]);
	}

	for (int i = 0; i < WIN_COUNT; i++) {
		win_draw(&windows[i], focus == i);
	}
}

static void cleanup_all (void) {
	for (int i = 0; i < WIN_COUNT; i++) {
		win_cleanup(&windows[i]);
	}
}

static void log_update (void) {
	char tmp[256];
	sprintf(tmp, "%i", rand());
	text_push(&windows[0].text, tmp);
	win_update_inside(&windows[0]);
}

int main (int argc, const char **argv) {
	setlocale(LC_ALL, "");

	WINDOW *root = initscr();
	cbreak();
	noecho();
	halfdelay(5);

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

	// Dialoge fit text
	win_set_post_update_action(&windows[3], WIN_UPDATE_ACTION_FIT_TEXT);

	win_show(&windows[2]);
	win_show(&windows[3]);

	int focus = 2;

	update_all(focus);

	move(3, 0);

	int chbuf[3] = {0};

	while (1) {
		int focus_change = 0;

		chbuf[0] = chbuf[1];
		chbuf[1] = chbuf[2];
		chbuf[2] = getch();

//		printf("%i-%i-%i\n", chbuf[0], chbuf[1], chbuf[2]);

		if (chbuf[0] == 27 && chbuf[1] == 91 && chbuf[2] == 90) {
			/* SHIFT TAB */
			focus_change = 1;
		}
		else {
			switch (chbuf[2]) {
				case 'l':
					win_show(&windows[0]);
					update_all(focus);
					break;
				case 'L':
					win_hide(&windows[0]);
					update_all(focus);
					break;
				case 's':
					win_show(&windows[1]);
					update_all(focus);
					break;
				case 'S':
					win_hide(&windows[1]);
					update_all(focus);
					break;
				case 'q':
					goto quit;
				case 9:
					/* TAB */
					focus_change = -1;
					break;
				case KEY_RESIZE:
					update_all(focus);
					break;
				default:
					break;
			}
		}

		if (focus_change) {
			focus += focus_change;
			for (int i = 0; i < WIN_COUNT; i++) {
				if (focus == WIN_COUNT)
					focus = 0;
				else if (focus < 0)
					focus = WIN_COUNT - 1;

				if (windows[focus].showing)
					break;

				focus += focus_change;
			}
			update_all(focus);
		}

		log_update();
		move(3, 0);
	}

	quit:

	endwin();
	exit_curses(0);
}
