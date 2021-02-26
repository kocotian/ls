#include <stdio.h>
#include <string.h>

#include "grammar.h"

extern char *contents;
extern char *output;

static char *
g_typetostr(TokenType type)
{
	switch (type) {
#include "tokentype.c"
	default:
		return "<unknown>"; break;
	}
}

static int
g_expecttype(Token token, TokenType type)
{
	if (token.type != type)
		errwarn("expected \033[1m%s\033[0m, got \033[1m%s\033[0m", 1,
				token, g_typetostr(type), g_typetostr(token.type));
	return 0;
}

size_t
g_statement(Token *tokens, size_t toksize)
{
	size_t i;
	i = 0;
	if (tokens[i].type == TokenOpeningBrace) { /* compound */
		++i;
		while (tokens[i].type != TokenClosingBrace && i < toksize) {
			g_statement(tokens + i, toksize - i); ++i;
		}
	} else if (tokens[i].type == TokenKeyword) {
		if        (!strncmp(contents + tokens[i].off, "if",     2)) { /* conditional */
		} else if (!strncmp(contents + tokens[i].off, "while",  5)) { /* loop */
		} else if (!strncmp(contents + tokens[i].off, "return", 6)) { /* return */
		} else if (!strncmp(contents + tokens[i].off, "var",    3)) { /* variable */
		} else if (!strncmp(contents + tokens[i].off, "const",  5)) { /* constant */
		} else {
			char buf[128];
			snprintf(buf, tokens[i].len + 1, "%s", contents + tokens[i].off);
			errwarn("unexpected keyword: \033[1m%s\033[0m", 1,
					tokens[i], buf);
		}
	} else if (!strncmp(contents + tokens[i].off, "void", 4)) { /* expression */
	} else if (tokens[i].type == TokenSemicolon) { /* noop */
	} else {
		errwarn("unexpected token \033[1m%s\033[0m, in place of a statement", 1,
				tokens[i], g_typetostr(tokens[i].type));
	}
}

size_t
g_function(Token *tokens, size_t toksize)
{
	size_t i;
	i = 0;
	fputs("identifier\n", stderr);
	g_expecttype(tokens[i++], TokenIdentifier);
	fputs("parenthesis (opening)\n", stderr);
	g_expecttype(tokens[i], TokenOpeningParenthesis);
	fputs("loop:\n", stderr);
	if (tokens[i + 1].type == TokenKeyword
	&& !strncmp(contents + tokens[i + 1].off, "void", 4))
		i += 2;
	else do {
		++i;
		fputs("\tidentifier\n", stderr);
		g_expecttype(tokens[i++], TokenIdentifier);
	} while (tokens[i].type == TokenComma);
	fputs("parenthesis (closing)\n", stderr);
	g_expecttype(tokens[i++], TokenClosingParenthesis);
	g_statement(tokens + i, toksize - i);
	++i;
	return i;
}

size_t
g_main(Token *tokens, size_t toksize)
{
	size_t i;
	for (i = 0; i < toksize; ++i) {
		i += g_function(tokens + i, toksize - i);
	}
}
