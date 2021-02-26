/* See licenses/LIBSL file for copyright and license details. */
/* Extended by kocotian */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		die("calloc:");
	return p;
}

void
die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(1);
}

ssize_t
nextline(int fd, char *buf, size_t size)
{
	size_t count, rb;
	int c;

	if (!size)
		return 0;

	for (count = c = 0; c != '\n' && count < size - 1; count++) {
		if (!((rb = read(fd, &c, 1)) > 0)) {
			if (count == 0)
				return -1;
			break;
		}

		buf[count] = c;
	}

	buf[count] = '\0';
	return (ssize_t)count;
}

void
errwarn(const char *fmt, int iserror, Token token, ...)
{
	extern const char *filename, *line;
	extern int fileline;

	va_list ap;
	fprintf(stderr, "\033[0;1m%s:%d:%d: \033[1;3%s: \033[0m",
			token.file, token.line, token.col, iserror ? "1merror" : "3mwarning");

	va_start(ap, iserror);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n% 5d | %s%c", fileline, line,
			line[strlen(line) - 1] != '\n' ? '\n' : 0);

	if (iserror) exit(1);
}
