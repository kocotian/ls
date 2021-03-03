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

ssize_t
strchlen(const char *s, char c)
{
	char *p;
	if ((p = strchr(s, c)) == NULL)
		return strlen(s);
	return p - s;
}

ssize_t
strlchlen(const char *s, char c)
{
	char *p;
	if ((p = strchr(s, c)) == NULL)
		return strlen(s);
	while ((p = strchr(p, c) != NULL))
	return p - s;
}

void
errwarn(const char *fmt, int iserror, Token token, ...)
{
	extern char *contents;
	char *buf;

	va_list ap;
	fprintf(stderr, "\033[0;1m%s:%d:%d: \033[1;3%s: \033[0m",
			token.file, token.line, token.col, iserror ? "1merror" : "3mwarning");

	va_start(ap, token);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	buf = malloc(strchlen((contents + token.off) - (token.col - 1), '\n') + 1);
	snprintf(buf, strchlen((contents + token.off) - (token.col - 1), '\n') + 1,
			"%s", (contents + token.off) - (token.col - 1));
	fprintf(stderr, "\n% 5d | %s%c", token.line, buf,
			contents[token.off + token.len] != '\n' ? '\n' : 0);
	free(buf);

	if (iserror) exit(1);
}
