#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arg.h"
#include "syscalls.h"
#include "util.h"

#define BUFSIZ 8192

#define ISLOW(ch) ((ch) > 0x60 && (ch) < 0x7b)
#define ISUPP(ch) ((ch) > 0x40 && (ch) < 0x5b)
#define ISNUM(ch) ((ch) > 0x2f && (ch) < 0x3a)
#define ISUND(ch) ((ch) == 0x5f)
#define ISPAR(ch) ((ch) == 0x28 || (ch) == 0x29)
#define ISBRC(ch) ((ch) == 0x5b || (ch) == 0x5d)
#define ISQUOT(ch) ((ch) == 0x22)
#define ISCOMM(ch) ((ch) == 0x2c)
#define ISIGNORABLE(ch) ((ch) > 0x00 && (ch) < 0x21)

#define ISIDENSTARTCHAR(ch) (ISUND(ch) || ISLOW(ch) || ISUPP(ch))
#define ISIDENCHAR(ch) (ISIDENSTARTCHAR(ch) || ISNUM(ch))
#define ISNUMCHAR(ch) (ISNUM(ch) || ((ch) > 0x60 && (ch) < 0x67) || \
		((ch) > 0x40 && (ch) < 0x47))

typedef enum {
	TokenNull,
	TokenNumber, TokenIdentifier, TokenString,
	TokenComma,
	TokenParenthesis, TokenBracket
} TokenType;

typedef struct {
	char *val;
	size_t len;
	TokenType type;
} Token;

static int getsyscallbyname(char *name);
static ssize_t parseline(char *input, size_t ilen, char *output, size_t olen,
                         int lnum, char *filename);
static void usage(void);

static void error(const char *fmt, const char *filename,
                  const char *line, int fileline, int filecol, ...);
static void warning(const char *fmt, const char *filename,
                    const char *line, int fileline, int filecol, ...);

char *argv0;

static int
getsyscallbyname(char *name)
{
	size_t i;
	for (i = 0; i < (sizeof(syscalls) / sizeof(*syscalls)); ++i)
		if (!strcmp(syscalls[i], name))
			return (int)i;
	return -1;
}

static ssize_t
parseline(char *input, size_t ilen, char *output, size_t olen, int lnum, char *filename)
{
	TokenType type;
	size_t i, j, li, tokiter;
	char ch, *valstart;
	*output = '\0';
	Token *tokens = malloc(sizeof(Token) * 128);

	for (tokiter = i = j = li = type = 0; i < ilen; ++i, ++j, ++li) {
		ch = input[i];
		if (!type) {
			valstart = input + i;
			if (ISNUM(ch))
				type = TokenNumber;
			else if (ISIDENSTARTCHAR(ch))
				type = TokenIdentifier;
			else if (ISQUOT(ch))
				type = TokenString;
			else if (ISIGNORABLE(ch)) {
				--j;
				continue;
			} else if (ISPAR(ch) || ISBRC(ch) || ISCOMM(ch)) {
				tokens[tokiter].val = valstart;
				tokens[tokiter].len = j + 1;
				tokens[tokiter++].type =
					ISPAR(ch) ? TokenParenthesis :
					ISBRC(ch) ? TokenBracket :
					TokenComma;
				type = TokenNull;
				j = -1;
			} else
				error("unexpected character: \033[1m%c \033[0m(\033[1m\\%o\033[0m)",
						filename, input, lnum, i + 1, ch, ch & 0xff);
		} else if ((type == TokenNumber     && !ISNUMCHAR(ch))
		|| (type == TokenIdentifier && !ISIDENCHAR(ch))
		|| (type == TokenString     && ISQUOT(ch))) {
			tokens[tokiter].val = valstart;
			tokens[tokiter].len = j;
			tokens[tokiter++].type = type;
			if (type != TokenString) --i;
			type = TokenNull;
			j = -1;
		}
	}

	free(tokens);
	return i;
}

static void
usage(void)
{
	die("usage: %s", argv0);
}

static void
error(const char *fmt, const char *filename, const char *line,
      int fileline, int filecol, ...)
{
	va_list ap;
	fprintf(stderr, "\033[0;1m%s:%d:%d: \033[1;31merror: \033[0m",
			filename, fileline, filecol);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n% 5d | %s%c", fileline, line,
			line[strlen(line) - 1] != '\n' ? '\n' : 0);

	exit(1);
}

static void
warning(const char *fmt, const char *filename, const char *line,
        int fileline, int filecol, ...)
{
	va_list ap;
	fprintf(stderr, "\033[0;1m%s:%d:%d: \033[1;33mwarning: \033[0m",
			filename, fileline, filecol);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n% 5d | %s", line);
}

int
main(int argc, char *argv[])
{
	char buffer[BUFSIZ], rbuf[BUFSIZ], cbuf[BUFSIZ];
	ssize_t rb;
	int lindex;

	void *data, *bss, *text;

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	for (rb = lindex = 0; (rb = nextline(0, buffer, BUFSIZ)) > 0; ++lindex) {
		parseline(buffer, rb, rbuf, BUFSIZ,
				lindex + 1, "<stdin>");
	}
}
