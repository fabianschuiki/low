/* Copyright (c) 2015-2016 Fabian Schuiki, Thomas Richner */
#pragma once
#include "grammar.h"
#include <stdlib.h>

typedef struct lexer lexer_t;

enum {
	TKN_INVALID = 0,
	TKN_SOF = 1,
	TKN_EOF = 2,

	#define TKN(id,name) TKN_##id,
	#include "lexer_tokens.h"
	TOKENS
	#undef TKN

	MAX_TOKENS
};

extern const char *token_names[];

struct lexer {
	const char *base;
	const char *ptr;
	const char *end;
	int token;
	loc_t loc;
	const char *line_base;
};

void lexer_init(lexer_t *self, const char *ptr, size_t len, const char *filename);
void lexer_next(lexer_t *self);
