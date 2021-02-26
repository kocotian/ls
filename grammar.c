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
g_expression(Token *tokens, size_t toksize)
{
	size_t i;
	i = 0;

	/* temporarily, expression can be a number only; TODO */
	g_expecttype(tokens[i++], TokenNumber);

	return i;
}

size_t
g_statement(Token *tokens, size_t toksize)
{
	size_t i;
	i = 0;
	if (tokens[i].type == TokenOpeningBrace) { /* compound */
		++i;
		while (tokens[i].type != TokenClosingBrace && i < toksize) {
			i += g_statement(tokens + i, toksize - i); ++i;
		}
	} else if (tokens[i].type == TokenKeyword) {
		if        (!strncmp(contents + tokens[i].off, "if",     2)) { /* conditional */
			g_expecttype(tokens[++i], TokenOpeningParenthesis);
			++i;
			i += g_expression(tokens + i, toksize - i);
			g_expecttype(tokens[i++], TokenClosingParenthesis);
			i += g_statement(tokens + i, toksize - i);
		} else if (!strncmp(contents + tokens[i].off, "while",  5)) { /* loop */
			g_expecttype(tokens[++i], TokenOpeningParenthesis);
			++i;
			i += g_expression(tokens + i, toksize - i);
			g_expecttype(tokens[i++], TokenClosingParenthesis);
			i += g_statement(tokens + i, toksize - i);
		} else if (!strncmp(contents + tokens[i].off, "return", 6)) { /* return */
			++i;
			i += g_expression(tokens + i, toksize - i);
			g_expecttype(tokens[i], TokenSemicolon);
		} else if (!strncmp(contents + tokens[i].off, "var",    3)) { /* variable */
			do {
				++i;
				g_expecttype(tokens[i++], TokenIdentifier);
				if (tokens[i].type == TokenEqualSign) {
					++i;
					i += g_expression(tokens + i, toksize - i);
				}
			} while (tokens[i].type == TokenComma);
			g_expecttype(tokens[i], TokenSemicolon);
		} else if (!strncmp(contents + tokens[i].off, "const",  5)) { /* constant */
			do {
				++i;
				g_expecttype(tokens[i++], TokenIdentifier);
				if (tokens[i].type == TokenEqualSign) {
					++i;
					i += g_expression(tokens + i, toksize - i);
				}
			} while (tokens[i].type == TokenComma);
			g_expecttype(tokens[i], TokenSemicolon);
		} else {
			char buf[128];
			snprintf(buf, tokens[i].len + 1, "%s", contents + tokens[i].off);
			errwarn("unexpected keyword: \033[1m%s\033[0m", 1,
					tokens[i], buf);
		}
	} else if (!strncmp(contents + tokens[i].off, "void", 4)) { /* expression */
	} else if (tokens[i].type == TokenSemicolon) { /* noop */
		++i;
	} else {
		i += g_expression(tokens + i, toksize - i);
		g_expecttype(tokens[i], TokenSemicolon);
	}

	return i;
}

size_t
g_function(Token *tokens, size_t toksize)
{
	size_t i;
	i = 0;
	g_expecttype(tokens[i++], TokenIdentifier);
	g_expecttype(tokens[i], TokenOpeningParenthesis);
	if (tokens[i + 1].type == TokenKeyword
	&& !strncmp(contents + tokens[i + 1].off, "void", 4))
		i += 2;
	else do {
		++i;
		g_expecttype(tokens[i++], TokenIdentifier);
	} while (tokens[i].type == TokenComma);
	g_expecttype(tokens[i++], TokenClosingParenthesis);
	i += g_statement(tokens + i, toksize - i);

	return ++i;
}

size_t
g_main(Token *tokens, size_t toksize)
{
	size_t i;
	for (i = 0; i < toksize; ++i) {
		i += g_function(tokens + i, toksize - i);
	}

	return i;
}
