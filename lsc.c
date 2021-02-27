/* See AUTHORS file for copyright details
 * and LICENSE file for license details.
 *
 * LinuxScript compiler is a simple, small compiler
 * for LinuxScript programming language wrote in
 * a suckless way.
 *
 * Consult README for more information about a language
 */

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
#define ISOPPAR(ch) ((ch) == 0x28)
#define ISOPBRK(ch) ((ch) == 0x5b)
#define ISOPBRC(ch) ((ch) == 0x7b)
#define ISCLPAR(ch) ((ch) == 0x29)
#define ISCLBRK(ch) ((ch) == 0x5d)
#define ISCLBRC(ch) ((ch) == 0x7d)
#define ISQUOT(ch) ((ch) == 0x22)
#define ISCOMM(ch) ((ch) == 0x2c)
#define ISEQUSIGN(ch) ((ch) == 0x3d)
#define ISSEMICOLON(ch) ((ch) == 0x3b)

#define ISIGNORABLE(ch) ((ch) > 0x00 && (ch) < 0x21)
#define ISLINECOMMSTARTCHAR(ch) ((ch) == 0x23)

#define ISIDENSTARTCHAR(ch) (ISUND(ch) || ISLOW(ch) || ISUPP(ch))
#define ISIDENCHAR(ch) (ISIDENSTARTCHAR(ch) || ISNUM(ch))
#define ISNUMCHAR(ch) (ISNUM(ch) || ((ch) > 0x60 && (ch) < 0x67) || \
		((ch) > 0x40 && (ch) < 0x47))

#include "lsc.h"
#include "grammar.h"

char *argv0;

char *filename, *line;
int fileline;

char *contents;
char *output;
size_t outsiz;

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
			(*tokens)[*tokiter].file = filename;
			(*tokens)[*tokiter].line = fileline;
			(*tokens)[*tokiter].col = valstart - off + 1;
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
			} else if (ISOPPAR(ch) || ISOPBRK(ch) || ISOPBRC(ch)
					|| ISCLPAR(ch) || ISCLBRK(ch) || ISCLBRC(ch)
					|| ISCOMM(ch) || ISSEMICOLON(ch) || ISEQUSIGN(ch)) {
				(*tokens)[*tokiter].off = valstart;
				(*tokens)[*tokiter].len = j + 1;
				(*tokens)[(*tokiter)++].type =
					ISOPPAR(ch) ? TokenOpeningParenthesis :
					ISOPBRK(ch) ? TokenOpeningBracket :
					ISOPBRC(ch) ? TokenOpeningBrace :
					ISCLPAR(ch) ? TokenClosingParenthesis :
					ISCLBRK(ch) ? TokenClosingBracket :
					ISCLBRC(ch) ? TokenClosingBrace :
					ISCOMM(ch) ? TokenComma :
					ISSEMICOLON(ch) ? TokenSemicolon :
					TokenEqualSign;
				type = TokenNull;
				j = -1;
			} else
				errwarn("unexpected character: \033[1m%c \033[0m(\033[1m\\%o\033[0m)",
						1, (*tokens)[*tokiter], ch, ch & 0xff);
		} else if ((type == TokenNumber && !ISNUMCHAR(ch))
		|| (type == TokenIdentifier && !ISIDENCHAR(ch))
		|| (type == TokenString && ISQUOT(ch))) {
			(*tokens)[*tokiter].file = filename;
			(*tokens)[*tokiter].line = fileline;
			(*tokens)[*tokiter].col = valstart - off + 1;
			(*tokens)[*tokiter].off = valstart;
			(*tokens)[*tokiter].len = j + (type == TokenString ? 1 : 0);
			if (!strncmp(input + (valstart - off), "if", 2)
			||  !strncmp(input + (valstart - off), "while", 5)
			||  !strncmp(input + (valstart - off), "return", 6)
			||  !strncmp(input + (valstart - off), "var", 3)
			||  !strncmp(input + (valstart - off), "const", 5)
			||  !strncmp(input + (valstart - off), "void", 4))
				type = TokenKeyword;
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
	char buffer[BUFSIZ];
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
			write(2, contents + tokens[j].off, tokens[j].len);
			write(2, &space, 1);
		}
		write(2, "\n", 1);
	}


	output = malloc(outsiz = snprintf(buffer, BUFSIZ, "BITS 64\n"
				"section .text\nglobal _start\n_start:\n"
				"\tcall main\n\tmov rax, 60\n\tmov rdi, 0\n"
				"\tsyscall\n\tret\n") + 1);

	memcpy(output, buffer, outsiz);
	memcpy(output + outsiz, "", 1);

	g_main(tokens, tokiter);
	write(1, output, outsiz - 1);

	free(output);
	free(tokens);
	free(contents);
}
