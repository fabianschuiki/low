/* Copyright (c) 2015-2016 Fabian Schuiki, Thomas Richner */
#include "lexer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


const char *token_names[MAX_TOKENS] = {
	#define TKN(id,name) [TKN_##id] = name,
	#include "lexer_tokens.h"
	TOKENS
	#undef TKN
};


static int
is_whitespace (char c) {
	return c <= 0x20;
}


static void
lexer_step(lexer_t *self) {
	if (self->ptr != self->end && *self->ptr == '\n') {
		++self->loc.line;
		self->loc.col = 0;
		self->line_base = self->ptr+1;
	}
	++self->ptr;
}


static void
lexer_steps(lexer_t *self, unsigned steps) {
	for (; steps > 0; --steps)
		lexer_step(self);
}


static void
consume_oneline_comment (lexer_t *self) {
	while (self->ptr != self->end) {
		if (*self->ptr == '\n') {
			lexer_step(self);
			return;
		} else {
			lexer_step(self);
		}
	}
}


static void
consume_multiline_comment (lexer_t *self) {
	while (self->ptr != self->end) {
		if (*self->ptr == '*') {
			lexer_step(self);
			if (self->ptr != self->end && *self->ptr == '/') {
				lexer_step(self);
				return;
			}
		} else {
			lexer_step(self);
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
is_number_dec (int c) {
	return (c >= '0' && c <= '9');
}

static int
is_number_start (int c) {
	return is_number_dec(c) || (c == '.');
}

static int
is_number (int c) {
	return is_number_start(c) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void
lexer_init (lexer_t *self, const char *ptr, size_t len, const char *filename) {
	bzero(self, sizeof *self);
	self->ptr = ptr;
	self->end = ptr+len;
	self->token = TKN_SOF;
	self->loc.filename = filename;
	self->line_base = ptr;
}

/*
	Requires semicolon:
	break continue fallthrough return ++ -- ) } <identifiers>
*/

static const unsigned terminators[] = {
	TKN_BREAK,
	TKN_CONTINUE,
	TKN_DEC_OP,
	TKN_IDENT,
	TKN_INC_OP,
	TKN_NUMBER_LITERAL,
	TKN_RBRACE,
	TKN_RETURN,
	TKN_RPAREN,
	TKN_STRING_LITERAL,
	TKN_VOID,
	TKN_RBRACK,
	0
};

void
lexer_next (lexer_t *self) {
	assert(self);
	assert(self->ptr && self->ptr <= self->end);

	if (self->token == TKN_INVALID || self->token == TKN_EOF)
		return;

	while (self->ptr != self->end) {
		if (is_whitespace(*self->ptr)) {
			/* no semicolon logic */
			if(*self->ptr=='\n'){
				int i;
				for(i=0;terminators[i]!=0;i++){
					if(self->token==terminators[i]){
						self->token=TKN_SEMICOLON;
						self->base = self->ptr;
						self->loc.col = (unsigned)(self->base - self->line_base);
						lexer_step(self);
						return;
					}
				}
			}
			lexer_step(self);
			continue;
		}
		self->base = self->ptr;
		self->loc.col = (unsigned)(self->base - self->line_base);

		unsigned rem = (self->end - self->ptr);

		if (rem >= 3) {
			#define CHECK_TOKEN(str,tkn) if (strncmp(str, self->ptr, 3) == 0) { lexer_steps(self, 3); self->token = tkn; return; }
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
			#define CHECK_TOKEN(str,tkn) if (strncmp(str, self->ptr, 2) == 0) { lexer_steps(self, 2); self->token = tkn; return; }
			CHECK_TOKEN(":=", TKN_DEF_ASSIGN)
			CHECK_TOKEN("+=", TKN_ADD_ASSIGN)
			CHECK_TOKEN("-=", TKN_SUB_ASSIGN)
			CHECK_TOKEN("*=", TKN_MUL_ASSIGN)
			CHECK_TOKEN("/=", TKN_DIV_ASSIGN)
			CHECK_TOKEN("%=", TKN_MOD_ASSIGN)
			CHECK_TOKEN("&=", TKN_AND_ASSIGN)
			CHECK_TOKEN("^=", TKN_XOR_ASSIGN)
			CHECK_TOKEN("|=", TKN_OR_ASSIGN)
			CHECK_TOKEN(">>", TKN_RIGHT_OP)
			CHECK_TOKEN("<<", TKN_LEFT_OP)
			CHECK_TOKEN("++", TKN_INC_OP)
			CHECK_TOKEN("--", TKN_DEC_OP)
			CHECK_TOKEN("&&", TKN_AND_OP)
			CHECK_TOKEN("||", TKN_OR_OP)
			CHECK_TOKEN("<=", TKN_LE_OP)
			CHECK_TOKEN(">=", TKN_GE_OP)
			CHECK_TOKEN("==", TKN_EQ_OP)
			CHECK_TOKEN("!=", TKN_NE_OP)
			CHECK_TOKEN("->", TKN_MAPTO)
			#undef CHECK_TOKEN
		}

		if (rem >= 1) {
			#define CHECK_TOKEN(chr,tkn) if (*self->ptr == chr) { lexer_step(self); self->token = tkn; return; }
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
			CHECK_TOKEN('!', TKN_NOT_OP)
			CHECK_TOKEN('~', TKN_BITWISE_NOT)
			CHECK_TOKEN('-', TKN_SUB_OP)
			CHECK_TOKEN('+', TKN_ADD_OP)
			CHECK_TOKEN('*', TKN_MUL_OP)
			CHECK_TOKEN('/', TKN_DIV_OP)
			CHECK_TOKEN('%', TKN_MOD_OP)
			CHECK_TOKEN('<', TKN_LT_OP)
			CHECK_TOKEN('>', TKN_GT_OP)
			CHECK_TOKEN('^', TKN_BITWISE_XOR)
			CHECK_TOKEN('|', TKN_BITWISE_OR)
			CHECK_TOKEN('?', TKN_QUESTION)
			CHECK_TOKEN('#', TKN_HASH)
			#undef CHECK_TOKEN
		}

		if (*self->ptr == '\"') {
			self->token = TKN_STRING_LITERAL;
			lexer_step(self);
			while (self->ptr != self->end) {
				if (*self->ptr == '"') {
					lexer_step(self);
					return;
				} else if (*self->ptr == '\\') {
					lexer_step(self);
					if (self->ptr == self->end) {
						fprintf(stderr, "unexpected end of file in the middle of escape sequence\n");
						self->token = TKN_INVALID;
						return;
					}
					lexer_step(self);
				} else {
					lexer_step(self);
				}
			}
			fprintf(stderr, "unexpected end of file in the middle of string literal\n");
			self->token = TKN_INVALID;
			return;
		}

		if (is_number_start(*self->ptr)) {
			lexer_step(self);

			while (is_number(*self->ptr) || (*self->ptr == '\''))
				lexer_step(self);
			self->token = TKN_NUMBER_LITERAL;
			return;
		}

		if (is_ident_start(*self->ptr)) {
			lexer_step(self);
			while (is_ident(*self->ptr))
				lexer_step(self);
			self->token = TKN_IDENT;
		}

		if (self->token == TKN_IDENT) {
			#define CHECK_KEYWORD(kw,tkn) if (match_keyword(kw, self->base, self->ptr)) self->token = tkn;
			CHECK_KEYWORD("alignas",   TKN_ALIGNAS);
			CHECK_KEYWORD("atomic",    TKN_ATOMIC);
			CHECK_KEYWORD("break",     TKN_BREAK);
			CHECK_KEYWORD("cap",       TKN_CAP);
			CHECK_KEYWORD("case",      TKN_CASE);
			CHECK_KEYWORD("const",     TKN_CONST);
			CHECK_KEYWORD("continue",  TKN_CONTINUE);
			CHECK_KEYWORD("default",   TKN_DEFAULT);
			CHECK_KEYWORD("do",        TKN_DO);
			CHECK_KEYWORD("else",      TKN_ELSE);
			CHECK_KEYWORD("enum",      TKN_ENUM);
			CHECK_KEYWORD("extern",    TKN_EXTERN);
			CHECK_KEYWORD("for",       TKN_FOR);
			CHECK_KEYWORD("free",      TKN_FREE);
			CHECK_KEYWORD("func",      TKN_FUNC);
			CHECK_KEYWORD("goto",      TKN_GOTO);
			CHECK_KEYWORD("if",        TKN_IF);
			CHECK_KEYWORD("import",    TKN_IMPORT);
			CHECK_KEYWORD("inline",    TKN_INLINE);
			CHECK_KEYWORD("interface", TKN_INTERFACE);
			CHECK_KEYWORD("len",       TKN_LEN);
			CHECK_KEYWORD("dispose",   TKN_DISPOSE);
			CHECK_KEYWORD("make",      TKN_MAKE);
			CHECK_KEYWORD("new",       TKN_NEW);
			CHECK_KEYWORD("return",    TKN_RETURN);
			CHECK_KEYWORD("sizeof",    TKN_SIZEOF);
			CHECK_KEYWORD("static",    TKN_STATIC);
			CHECK_KEYWORD("struct",    TKN_STRUCT);
			CHECK_KEYWORD("switch",    TKN_SWITCH);
			CHECK_KEYWORD("type",      TKN_TYPE);
			CHECK_KEYWORD("typedef",   TKN_TYPEDEF);
			CHECK_KEYWORD("union",     TKN_UNION);
			CHECK_KEYWORD("var",       TKN_VAR);
			CHECK_KEYWORD("void",      TKN_VOID);
			CHECK_KEYWORD("volatile",  TKN_VOLATILE);
			CHECK_KEYWORD("while",     TKN_WHILE);
			CHECK_KEYWORD("yield",     TKN_YIELD);
			#undef CHECK_KEYWORD
			return;
		}

		fprintf(stderr, "ignoring garbage character '%c'\n", *self->ptr);
		lexer_step(self);
	}

	self->token = TKN_EOF;
}
