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

#define ERROR(str, filename, fileline, ...) \
	printf("\033[1;97m%s:%d: \033[0m""%s"str"\n", \
			filename, fileline, errstr, __VA_ARGS__)
#define WARNING(str, filename, fileline, ...) \
	printf("\033[1;97m%s:%d: \033[0m""%s"str"\n", \
			filename, fileline, warnstr, __VA_ARGS__)

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
static void usage(void);

char *argv0;
const char *warnstr = "\033[1;33mwarning: \033[0m";
const char *errstr = "\033[1;31merror: \033[0m";

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
parseline(char *input, size_t ilen, char *output, size_t olen, int lineno)
{
	/* === Types ===
	   TokenNull,
	   TokenNumber, TokenIdentifier, TokenString,
	   TokenComma,
	   TokenParenthesis, TokenBracket */

	TokenType type;
	size_t i, j, li, tokiter;
	char ch, *valstart;
	*output = '\0';
	Token *tokens = malloc(sizeof(Token) * 128);

	valstart = NULL;
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
			}
		} else {
			if ((type == TokenNumber     && !ISNUMCHAR(ch))
			||  (type == TokenIdentifier && !ISIDENCHAR(ch))
			||  (type == TokenString     && ISQUOT(ch))) {
				tokens[tokiter].val = valstart;
				tokens[tokiter].len = j;
				tokens[tokiter++].type = type;
				if (type != TokenString) --i;
				type = TokenNull;
				j = -1;
			}
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
		write(1, buffer, rb);
		parseline(buffer, rb, rbuf, BUFSIZ, lindex + 1);
	}
}
