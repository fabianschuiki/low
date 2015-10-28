/* Copyright (c) 2015 Fabian Schuiki */
#pragma once

typedef struct rule rule_t;
typedef struct variant variant_t;
typedef struct token token_t;

typedef void(*reduce_fn_t)(const token_t*, unsigned, void*);

struct variant {
	const rule_t **elements;
	reduce_fn_t reducer;
	void *reducer_arg;
};

struct rule {
	const char *name;
	const variant_t *variants;
};

struct token {
	int token;
	const rule_t *rule;
	union {
		struct {
			const char *first;
			const char *last;
		} lex;
	};
};

extern const rule_t grammar_root;
