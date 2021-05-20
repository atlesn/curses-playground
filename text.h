#ifndef TEXT_H
#define TEXT_H

#define TEXT_MAX_LINES 256

struct text {
	char *lines[TEXT_MAX_LINES];
	int latest;
};

struct win;

void text_cleanup(struct text *text);
void text_push(struct text *text, const char *str);
void text_print (
		const int line_max,
		const int rows_max,
		struct text *text,
		void (*callback)(const char *text, const int row, void *callback_arg),
		void *callback_arg
);
int text_get_longest(struct text *text);

#endif /* TEXT_H */
