#include <string.h>
#include <stdlib.h>

#include "text.h"

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

void text_print (
		const int line_max,
		const int rows_max,
		struct text *text,
		void (*callback)(const char *text, const int row, void *callback_arg),
		void *callback_arg
) {
	if (!text->lines[text->latest])
		return;

	char tmp[line_max + 1];

	int pos = text->latest;
	for (int row = rows_max - 1; row >= 0; row--) {
		memset(tmp, ' ', line_max);
		tmp[line_max] = '\0';

		if (text->lines[pos]) {
			size_t len = strlen(text->lines[pos]);
			memcpy(tmp, text->lines[pos], len < line_max ? len : line_max);
		}

		callback(tmp, row, callback_arg);

		if (--pos < 0)
			pos = TEXT_MAX_LINES - 1;
	}
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
