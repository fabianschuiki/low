/* Copyright (c) 2015 Fabian Schuiki */
#pragma once
#include "grammar.h"
#include <stdlib.h>

typedef struct lexer lexer_t;

enum {
	TKN_INVALID = 0,
	TKN_SOF = 1,
	TKN_EOF = 2,

	// literals
	TKN_IDENT,
	TKN_NUMBER_LITERAL,
	TKN_STRING_LITERAL,

	// operators
	TKN_ADD_OP,
	TKN_AND_OP,
	TKN_BITWISE_AND,
	TKN_BITWISE_NOT,
	TKN_BITWISE_OR,
	TKN_BITWISE_XOR,
	TKN_DEC_OP,
	TKN_DIV_OP,
	TKN_EQ_OP,
	TKN_GE_OP,
	TKN_GT_OP,
	TKN_INC_OP,
	TKN_LEFT_OP,
	TKN_LE_OP,
	TKN_LT_OP,
	TKN_MAPTO,
	TKN_MOD_OP,
	TKN_MUL_OP,
	TKN_NE_OP,
	TKN_NOT_OP,
	TKN_OR_OP,
	TKN_RIGHT_OP,
	TKN_SUB_OP,

	// assignment operators
	TKN_ASSIGN,
	TKN_ADD_ASSIGN,
	TKN_AND_ASSIGN,
	TKN_DIV_ASSIGN,
	TKN_LEFT_ASSIGN,
	TKN_MOD_ASSIGN,
	TKN_MUL_ASSIGN,
	TKN_OR_ASSIGN,
	TKN_RIGHT_ASSIGN,
	TKN_SUB_ASSIGN,
	TKN_XOR_ASSIGN,

	// symbols
	TKN_COMMA,
	TKN_COLON,
	TKN_ELLIPSIS,
	TKN_HASH,
	TKN_LBRACE,
	TKN_LBRACK,
	TKN_LPAREN,
	TKN_PERIOD,
	TKN_QUESTION,
	TKN_RBRACE,
	TKN_RBRACK,
	TKN_RPAREN,
	TKN_SEMICOLON,

	// keywords
	TKN_ALIGNAS,
	TKN_ATOMIC,
	TKN_BREAK,
	TKN_CASE,
	TKN_CONST,
	TKN_CONTINUE,
	TKN_DEFAULT,
	TKN_DO,
	TKN_ELSE,
	TKN_ENUM,
	TKN_EXTERN,
	TKN_FOR,
	TKN_FUNC,
	TKN_GOTO,
	TKN_IF,
	TKN_IMPORT,
	TKN_INLINE,
	TKN_NEW,
	TKN_FREE,
	TKN_RETURN,
	TKN_SIZEOF,
	TKN_STATIC,
	TKN_STRUCT,
	TKN_SWITCH,
	TKN_TYPE,
	TKN_TYPEDEF,
	TKN_UNION,
	TKN_VAR,
	TKN_VOID,
	TKN_VOLATILE,
	TKN_WHILE,
	TKN_YIELD,

	MAX_TOKENS
};

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
