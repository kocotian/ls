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
#define ISBRK(ch) ((ch) == 0x5b || (ch) == 0x5d)
#define ISBRC(ch) ((ch) == 0x7b || (ch) == 0x7d)
#define ISQUOT(ch) ((ch) == 0x22)
#define ISCOMM(ch) ((ch) == 0x2c)
#define ISEQUSIGN(ch) ((ch) == 0x3d)
#define ISIGNORABLE(ch) ((ch) > 0x00 && (ch) < 0x21)
#define ISLINECOMMSTARTCHAR(ch) ((ch) == 0x3b || (ch) == 0x23)

#define ISIDENSTARTCHAR(ch) (ISUND(ch) || ISLOW(ch) || ISUPP(ch))
#define ISIDENCHAR(ch) (ISIDENSTARTCHAR(ch) || ISNUM(ch))
#define ISNUMCHAR(ch) (ISNUM(ch) || ((ch) > 0x60 && (ch) < 0x67) || \
		((ch) > 0x40 && (ch) < 0x47))

typedef enum {
	TokenNull,
	TokenNumber, TokenIdentifier, TokenString,
	TokenComma,
	TokenParenthesis, TokenBracket, TokenBrace,
	TokenEqualSign
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

static void errwarn(const char *fmt, int iserror, const char *filename,
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
			else if (ISLINECOMMSTARTCHAR(ch)) {
				break;
			} else if (ISIGNORABLE(ch)) {
				--j;
				continue;
			} else if (ISPAR(ch) || ISBRK(ch) || ISBRC(ch) || ISCOMM(ch) || ISEQUSIGN(ch)) {
				tokens[tokiter].val = valstart;
				tokens[tokiter].len = j + 1;
				tokens[tokiter++].type =
					ISPAR(ch) ? TokenParenthesis :
					ISBRK(ch) ? TokenBracket :
					ISBRC(ch) ? TokenBrace :
					ISCOMM(ch) ? TokenComma :
					TokenEqualSign;
				type = TokenNull;
				j = -1;
			} else
				errwarn("unexpected character: \033[1m%c \033[0m(\033[1m\\%o\033[0m)",
						1, filename, input, lnum, i + 1, ch, ch & 0xff);
		} else if ((type == TokenNumber && !ISNUMCHAR(ch))
		|| (type == TokenIdentifier && !ISIDENCHAR(ch))
		|| (type == TokenString && ISQUOT(ch))) {
			tokens[tokiter].val = valstart;
			tokens[tokiter].len = j + (type == TokenString ? 1 : 0);
			tokens[tokiter++].type = type;
			if (type != TokenString) --i;
			type = TokenNull;
			j = -1;
		}
	}

	const char space = ' ';
	for (j = 0; j < tokiter; ++j) {
		write(1, tokens[j].val, tokens[j].len);
		write(1, &space, 1);
	}
	puts("");

	free(tokens);
	return i;
}

static void
usage(void)
{
	die("usage: %s", argv0);
}

static void
errwarn(const char *fmt, int iserror, const char *filename, const char *line,
      int fileline, int filecol, ...)
{
	va_list ap;
	fprintf(stderr, "\033[0;1m%s:%d:%d: \033[1;3%s: \033[0m",
			filename, fileline, filecol, iserror ? "1merror" : "3mwarning");

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fprintf(stderr, "\n% 5d | %s%c", fileline, line,
			line[strlen(line) - 1] != '\n' ? '\n' : 0);

	if (iserror) exit(1);
}

int
main(int argc, char *argv[])
{
	char buffer[BUFSIZ], tmpbuf[BUFSIZ];
	ssize_t rb;
	int lindex;

	void *data, *bss, *text;

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	for (rb = lindex = 0; (rb = nextline(0, buffer, BUFSIZ)) > 0; ++lindex) {
		parseline(buffer, rb, tmpbuf, BUFSIZ,
				lindex + 1, "<stdin>");
	}
}
