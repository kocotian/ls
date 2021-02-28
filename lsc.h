/* See AUTHORS file for copyright details
   and LICENSE file for license details. */

#ifndef _LSC_H
#define _LSC_H

#include "tokentype.h"

typedef struct {
	short line, col;
	char *file;
	size_t off, len;
	TokenType type;
} Token;

#include "util.h"

int
getsyscallbyname(char *name);

#endif
