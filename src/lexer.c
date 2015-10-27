/* Copyright (c) 2015 Fabian Schuiki */
#include "lexer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


static int
is_whitespace (char c) {
	return c <= 0x20;
}


static void
consume_oneline_comment (lexer_t *self) {
	while (self->ptr != self->end) {
		if (*self->ptr == '\n') {
			++self->ptr;
			return;
		} else {
			++self->ptr;
		}
	}
}


static void
consume_multiline_comment (lexer_t *self) {
	while (self->ptr != self->end) {
		if (*self->ptr == '*') {
			++self->ptr;
			if (self->ptr != self->end && *self->ptr == '/') {
				++self->ptr;
				return;
			}
		} else {
			++self->ptr;
		}
	}
}


static int
is_ident_start (int c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}


static int
is_ident (int c) {
	return is_ident_start(c) || (c >= '0' && c <= '9');
}


static int
match_keyword (const char *kw, const char *ptr, const char *end) {
	for (; ptr < end && *kw == *ptr; ++kw, ++ptr);
	return *kw == 0 && ptr == end;
}


static int
is_number_start (int c) {
	return (c >= '0' && c <= '9') || (c == '.');
}


static int
is_number (int c) {
	return is_number_start(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}


void
lexer_init (lexer_t *self, const char *ptr, size_t len) {
	self->base = 0;
	self->ptr = ptr;
	self->end = ptr+len;
	self->token = TKN_SOF;
}


void
lexer_next (lexer_t *self) {
	assert(self);
	assert(self->ptr && self->ptr <= self->end);

	if (self->token == TKN_INVALID || self->token == TKN_EOF)
		return;

	while (self->ptr != self->end) {
		if (is_whitespace(*self->ptr)) {
			++self->ptr;
			continue;
		}
		self->base = self->ptr;

		unsigned rem = (self->end - self->ptr);

		if (rem >= 3) {
			#define CHECK_TOKEN(str,tkn) if (strncmp(str, self->ptr, 3) == 0) { self->ptr += 3; self->token = tkn; return; }
			CHECK_TOKEN("...", TKN_ELLIPSIS)
			CHECK_TOKEN(">>=", TKN_RIGHT_ASSIGN)
			CHECK_TOKEN("<<=", TKN_LEFT_ASSIGN)
			#undef CHECK_TOKEN
		}

		if (rem >= 2) {
			if (strncmp("//", self->ptr, 2) == 0) {
				self->ptr += 2;
				consume_oneline_comment(self);
				continue;
			}
			if (strncmp("/*", self->ptr, 2) == 0) {
				self->ptr += 2;
				consume_multiline_comment(self);
				continue;
			}
			#define CHECK_TOKEN(str,tkn) if (strncmp(str, self->ptr, 2) == 0) { self->ptr += 2; self->token = tkn; return; }
			CHECK_TOKEN("+=", TKN_ADD_ASSIGN)
			CHECK_TOKEN("-=", TKN_SUB_ASSIGN)
			CHECK_TOKEN("*=", TKN_MUL_ASSIGN)
			CHECK_TOKEN("/=", TKN_DIV_ASSIGN)
			CHECK_TOKEN("%=", TKN_MOD_ASSIGN)
			CHECK_TOKEN("&=", TKN_AND_ASSIGN)
			CHECK_TOKEN("^=", TKN_XOR_ASSIGN)
			CHECK_TOKEN("|=", TKN_OR_ASSIGN)
			CHECK_TOKEN(">>", TKN_RIGHT)
			CHECK_TOKEN("<<", TKN_LEFT)
			CHECK_TOKEN("++", TKN_INC)
			CHECK_TOKEN("--", TKN_DEC)
			CHECK_TOKEN("&&", TKN_AND)
			CHECK_TOKEN("||", TKN_OR)
			CHECK_TOKEN("<=", TKN_LE)
			CHECK_TOKEN(">=", TKN_GE)
			CHECK_TOKEN("==", TKN_EQ)
			CHECK_TOKEN("!=", TKN_NE)
			#undef CHECK_TOKEN
		}

		if (rem >= 1) {
			#define CHECK_TOKEN(chr,tkn) if (*self->ptr == chr) { ++self->ptr; self->token = tkn; return; }
			CHECK_TOKEN(';', TKN_SEMICOLON)
			CHECK_TOKEN('{', TKN_LBRACE)
			CHECK_TOKEN('}', TKN_RBRACE)
			CHECK_TOKEN(',', TKN_COMMA)
			CHECK_TOKEN(':', TKN_COLON)
			CHECK_TOKEN('=', TKN_ASSIGN)
			CHECK_TOKEN('(', TKN_LPAREN)
			CHECK_TOKEN(')', TKN_RPAREN)
			CHECK_TOKEN('[', TKN_LBRACK)
			CHECK_TOKEN(']', TKN_RBRACK)
			CHECK_TOKEN('.', TKN_PERIOD)
			CHECK_TOKEN('&', TKN_BITWISE_AND)
			CHECK_TOKEN('!', TKN_NOT)
			CHECK_TOKEN('~', TKN_BITWISE_NOT)
			CHECK_TOKEN('-', TKN_SUB)
			CHECK_TOKEN('+', TKN_ADD)
			CHECK_TOKEN('*', TKN_MUL)
			CHECK_TOKEN('/', TKN_DIV)
			CHECK_TOKEN('%', TKN_MOD)
			CHECK_TOKEN('<', TKN_LT)
			CHECK_TOKEN('>', TKN_GT)
			CHECK_TOKEN('^', TKN_BITWISE_XOR)
			CHECK_TOKEN('|', TKN_BITWISE_OR)
			CHECK_TOKEN('?', TKN_QUESTION)
			#undef CHECK_TOKEN
		}

		if (*self->ptr == '\"') {
			self->token = TKN_STRING_LITERAL;
			++self->ptr;
			while (self->ptr != self->end) {
				if (*self->ptr == '"') {
					++self->ptr;
					return;
				} else {
					++self->ptr;
				}
			}
			fprintf(stderr, "unexpected end of file in the middle of string literal\n");
			self->token = TKN_INVALID;
			return;
		}

		if (is_number_start(*self->ptr)) {
			++self->ptr;
			while (is_number(*self->ptr))
				++self->ptr;
			self->token = TKN_NUMBER_LITERAL;
			return;
		}

		if (is_ident_start(*self->ptr)) {
			++self->ptr;
			while (is_ident(*self->ptr))
				++self->ptr;
			self->token = TKN_IDENT;
		}

		if (self->token == TKN_IDENT) {
			#define CHECK_KEYWORD(kw,tkn) if (match_keyword(kw, self->base, self->ptr)) self->token = tkn;
			CHECK_KEYWORD("atomic",   TKN_ATOMIC);
			CHECK_KEYWORD("break",    TKN_BREAK);
			CHECK_KEYWORD("case",     TKN_CASE);
			CHECK_KEYWORD("const",    TKN_CONST);
			CHECK_KEYWORD("continue", TKN_CONTINUE);
			CHECK_KEYWORD("default",  TKN_DEFAULT);
			CHECK_KEYWORD("do",       TKN_DO);
			CHECK_KEYWORD("else",     TKN_ELSE);
			CHECK_KEYWORD("enum",     TKN_ENUM);
			CHECK_KEYWORD("extern",   TKN_EXTERN);
			CHECK_KEYWORD("for",      TKN_FOR);
			CHECK_KEYWORD("goto",     TKN_GOTO);
			CHECK_KEYWORD("if",       TKN_IF);
			CHECK_KEYWORD("inline",   TKN_INLINE);
			CHECK_KEYWORD("return",   TKN_RETURN);
			CHECK_KEYWORD("sizeof",   TKN_SIZEOF);
			CHECK_KEYWORD("static",   TKN_STATIC);
			CHECK_KEYWORD("struct",   TKN_STRUCT);
			CHECK_KEYWORD("switch",   TKN_SWITCH);
			CHECK_KEYWORD("typedef",  TKN_TYPEDEF);
			CHECK_KEYWORD("union",    TKN_UNION);
			CHECK_KEYWORD("void",     TKN_VOID);
			CHECK_KEYWORD("volatile", TKN_VOLATILE);
			CHECK_KEYWORD("while",    TKN_WHILE);
			CHECK_KEYWORD("yield",    TKN_YIELD);
			#undef CHECK_KEYWORD
			return;
		}

		fprintf(stderr, "ignoring garbage character '%c'\n", *self->ptr);
		++self->ptr;
	}

	self->token = TKN_EOF;
}
