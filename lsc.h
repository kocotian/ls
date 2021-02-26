#ifndef _LSC_H
#define _LSC_H

#include "tokentype.h"
#include "util.h"

typedef struct {
	short line, col;
	char *file;
	size_t off, len;
	TokenType type;
} Token;

static int
getsyscallbyname(char *name);
static ssize_t
parseline(char *input, size_t ilen, size_t off, Token **tokens, size_t *toksiz, size_t *tokiter);
static void
usage(void);

#endif
