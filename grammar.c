/* See AUTHORS file for copyright details
   and LICENSE file for license details. */

#include <stdio.h>
#include <string.h>

#include "grammar.h"

extern char    *contents;
extern char    *output;
extern size_t   outsiz;

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

	if (tokens[i].type == TokenNumber) { /* number literal */
		++i;
	} else if (tokens[i].type == TokenString) { /* string literal */
		++i;
	} else if (tokens[i].type == TokenIdentifier) { /* identifier literal */
		++i;
	} else if (tokens[i].type == TokenOpeningParenthesis) { /* (expression) */
		++i;
		i += g_expression(tokens + i, toksize - i);
		g_expecttype(tokens[i++], TokenClosingParenthesis);
	} else if (tokens[i].type == TokenMinusSign) { /* -sign-change */
		++i;
		i += g_expression(tokens + i, toksize - i);
	} else if (tokens[i].type == TokenExclamationMark) { /* !negation */
		++i;
		i += g_expression(tokens + i, toksize - i);
	} else if (tokens[i].type == TokenIncrement) { /* ++pre-incrementation */
		++i;
		i += g_expression(tokens + i, toksize - i);
	} else if (tokens[i].type == TokenDecrement) { /* --pre-decrementation */
		++i;
		i += g_expression(tokens + i, toksize - i);
	}

	/* expressions that starts with another expressions; TODO */
	{
		/* i += g_expression(tokens + i, toksize - i); */
		if (tokens[i].type == TokenQuestionMark) { /* ternary expression */
			++i;
			i += g_expression(tokens + i, toksize - i);
			g_expecttype(tokens[i++], TokenColon);
			i += g_expression(tokens + i, toksize - i);
		} else if (tokens[i].type == TokenIncrement) { /* post-incrementation++ */
			++i;
		} else if (tokens[i].type == TokenDecrement) { /* post-decrementation-- */
			++i;
		} else if (tokens[i].type == TokenOpeningBracket) { /* indexing[expr] */
			++i;
			i += g_expression(tokens + i, toksize - i);
			g_expecttype(tokens[i++], TokenClosingBracket);
		} else if (tokens[i].type == TokenOpeningParenthesis) { /* function(expr) */
			do {
				++i;
				i += g_expression(tokens + i, toksize - i);
			} while (tokens[i].type == TokenComma);
			g_expecttype(tokens[i++], TokenClosingParenthesis);
		} else {
			if (tokens[i].type == TokenAssignmentSign) { /* expr = expr */
				++i;
			} else if (tokens[i].type == TokenPlusSign) { /* expr + expr */
				++i;
			} else if (tokens[i].type == TokenMinusSign) { /* expr - expr */
				++i;
			} else if (tokens[i].type == TokenPlusEqualSign) { /* expr += expr */
				++i;
			} else if (tokens[i].type == TokenMinusEqualSign) { /* expr -= expr */
				++i;
			} else if (tokens[i].type == TokenLogicalOrSign) { /* expr || expr */
				++i;
			} else if (tokens[i].type == TokenLogicalAndSign) { /* expr && expr */
				++i;
			} else if (tokens[i].type == TokenLogicalEquSign) { /* expr == expr */
				++i;
			} else if (tokens[i].type == TokenLogicalNotEquSign) { /* expr != expr */
				++i;
			} else if (tokens[i].type == TokenComma) { /* expr, expr */
				++i;
			} else {
				return i;
			}
			i += g_expression(tokens + i, toksize - i);
		}
	}

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
				if (tokens[i].type == TokenAssignmentSign) {
					++i;
					i += g_expression(tokens + i, toksize - i);
				}
			} while (tokens[i].type == TokenComma);
			g_expecttype(tokens[i], TokenSemicolon);
		} else if (!strncmp(contents + tokens[i].off, "const",  5)) { /* constant */
			do {
				++i;
				g_expecttype(tokens[i++], TokenIdentifier);
				if (tokens[i].type == TokenAssignmentSign) {
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
	char *func_name;
	char buffer[BUFSIZ];

	size_t i, rb;
	i = 0;
	g_expecttype(tokens[i++], TokenIdentifier);
	func_name = strndup(contents + tokens[i - 1].off, tokens[i - 1].len);
	g_expecttype(tokens[i], TokenOpeningParenthesis);
	if (tokens[i + 1].type == TokenKeyword
	&& !strncmp(contents + tokens[i + 1].off, "void", 4))
		i += 2;
	else do {
		++i;
		g_expecttype(tokens[i++], TokenIdentifier);
	} while (tokens[i].type == TokenComma);
	g_expecttype(tokens[i++], TokenClosingParenthesis);

	rb = snprintf(buffer, BUFSIZ, "%s:\n", func_name);
	output = realloc(output, outsiz += rb);
	strcat(output, buffer);
	free(func_name);

	i += g_statement(tokens + i, toksize - i);

	rb = snprintf(buffer, BUFSIZ, "\tret\n");
	output = realloc(output, outsiz += rb);
	strcat(output, buffer);

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
