#include <stdarg.h>
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

#include "lsc.h"
#include "grammar.h"

char *argv0;

char *filename, *line;
int fileline;

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
parseline(char *input, size_t ilen, size_t off, Token **tokens, size_t *toksiz, size_t *tokiter)
{
	TokenType type;
	size_t i, j, li, valstart;
	char ch;

	line = input;

	for (i = j = li = type = 0; i < ilen; ++i, ++j, ++li) {
		if ((*tokiter >= (*toksiz - 1)))
			*tokens = realloc(*tokens, sizeof(Token) * (*toksiz += 128));
		ch = input[i];
		if (!type) {
			valstart = off + i;
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
				(*tokens)[*tokiter].file = filename;
				(*tokens)[*tokiter].line = fileline;
				(*tokens)[*tokiter].col = valstart - off + 1;
				(*tokens)[*tokiter].off = valstart;
				(*tokens)[*tokiter].len = j + 1;
				(*tokens)[(*tokiter)++].type =
					ISPAR(ch) ? TokenParenthesis :
					ISBRK(ch) ? TokenBracket :
					ISBRC(ch) ? TokenBrace :
					ISCOMM(ch) ? TokenComma :
					TokenEqualSign;
				type = TokenNull;
				j = -1;
			} else
				errwarn("unexpected character: \033[1m%c \033[0m(\033[1m\\%o\033[0m)",
						1, ch, ch & 0xff);
		} else if ((type == TokenNumber && !ISNUMCHAR(ch))
		|| (type == TokenIdentifier && !ISIDENCHAR(ch))
		|| (type == TokenString && ISQUOT(ch))) {
			(*tokens)[*tokiter].file = filename;
			(*tokens)[*tokiter].line = fileline;
			(*tokens)[*tokiter].col = valstart - off + 1;
			(*tokens)[*tokiter].off = valstart;
			(*tokens)[*tokiter].len = j + (type == TokenString ? 1 : 0);
			(*tokens)[(*tokiter)++].type = type;
			if (type != TokenString) --i;
			type = TokenNull;
			j = -1;
		}
	}

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
	char buffer[BUFSIZ], *contents;
	ssize_t rb;
	size_t csiz, toksiz, tokiter;
	int lindex;
	Token *tokens;

	/* void *data, *bss, *text; */

	ARGBEGIN {
	default:
		usage();
	} ARGEND

	contents = malloc(csiz = 0);
	tokens = malloc(sizeof(*tokens) * (toksiz = 128));

	filename = "<stdin>";

	for (rb = lindex = tokiter = 0; (rb = nextline(0, buffer, BUFSIZ)) > 0; ++lindex) {
		contents = realloc(contents, csiz += rb);
		memcpy(contents + (csiz - rb), buffer, rb);
		fileline = lindex + 1;
		parseline(contents + (csiz - rb), rb, (csiz - rb), &tokens, &toksiz, &tokiter);
	}

	{
		const char space = ' ';
		int j;
		for (j = 0; j < tokiter; ++j) {
			write(1, contents + tokens[j].off, tokens[j].len);
			write(1, &space, 1);
		}
		write(1, "\n", 1);
	}

	printf("tokiter: %d\n", tokiter);
	g_main(tokens, tokiter);

	free(tokens);
	free(contents);
}
