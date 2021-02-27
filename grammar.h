/* See AUTHORS file for copyright details
   and LICENSE file for license details. */

#include "lsc.h"

static int g_expecttype(Token token, TokenType type);

size_t g_function(Token *tokens, size_t toksize);
size_t g_main(Token *tokens, size_t toksize);
