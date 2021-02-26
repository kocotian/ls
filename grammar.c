#include <stdio.h>

#include "grammar.h"

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
	if (tokens[i].type == TokenBrace) {
		++i;
		while (tokens[i].type != TokenBrace && i < toksize);
			g_statement(tokens + i, toksize - i); ++i;
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
	g_expecttype(tokens[i], TokenParenthesis);
	fputs("loop:\n", stderr);
	do {
		++i;
		fputs("\tidentifier\n", stderr);
		g_expecttype(tokens[i++], TokenIdentifier);
	} while (tokens[i].type == TokenComma);
	fputs("parenthesis (closing)\n", stderr);
	g_expecttype(tokens[i++], TokenParenthesis);
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
