/* Copyright (c) 2015 Fabian Schuiki */
#include "grammar.h"
#include "lexer.h"
#include <stdio.h>
#include <string.h>


void
reduce_external_declaration (const token_t *tokens, unsigned num_tokens, void *arg) {
	printf("parsed external declaration\n");
}

void
reduce_import_statement_0 (const token_t *tokens, unsigned num_tokens, void *arg) {
	unsigned len = tokens[1].lex.last - tokens[1].lex.first;
	char buf[len+1];
	strncpy(buf, tokens[1].lex.first, len);
	printf("parsed file import %s\n", buf);
}

void
reduce_import_statement_1 (const token_t *tokens, unsigned num_tokens, void *arg) {
	printf("parsed void import\n");
}

void
reduce_import_statement_2 (const token_t *tokens, unsigned num_tokens, void *arg) {
	printf("parsed external declaration import\n");
}

void
reduce_translation_unit (const token_t *tokens, unsigned num_tokens, void *arg) {

}


extern const rule_t translation_unit;
extern const rule_t external_declaration;


const rule_t external_declaration = {
	"external_declaration", (const variant_t[]){
		{(const rule_t*[]){(void*)TKN_RETURN, (void*)TKN_STRING_LITERAL, 0}, reduce_external_declaration},
		{0}
	}
};

const rule_t import_statement = {
	"import_statement", (const variant_t[]){
		{(const rule_t*[]){(void*)TKN_IMPORT, (void*)TKN_STRING_LITERAL, (void*)TKN_SEMICOLON, 0}, reduce_import_statement_0},
		{(const rule_t*[]){(void*)TKN_IMPORT, (void*)TKN_VOID, (void*)TKN_SEMICOLON, 0}, reduce_import_statement_1},
		{(const rule_t*[]){(void*)TKN_IMPORT, &external_declaration, (void*)TKN_SEMICOLON, 0}, reduce_import_statement_2},
		{0}
	}
};

const rule_t translation_unit = {
	"translation_unit", (const variant_t[]){
		{(const rule_t*[]){&external_declaration, 0}, 0},
		{(const rule_t*[]){&import_statement, 0}, 0},
		{(const rule_t*[]){&translation_unit, &external_declaration, 0}, 0},
		{(const rule_t*[]){&translation_unit, &import_statement, 0}, 0},
		{0},
	}
};

const rule_t grammar_root = {
	"root", (const variant_t[]){
		{(const rule_t*[]){&translation_unit, 0}, 0},
		{0},
	}
};
