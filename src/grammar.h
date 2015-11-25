/* Copyright (c) 2015 Fabian Schuiki */
#pragma once

typedef struct loc loc_t;
typedef struct variant variant_t;
typedef struct rule rule_t;
typedef struct token token_t;

typedef void(*reduce_fn_t)(token_t *, const token_t*, int);

struct loc {
	const char *filename;
	unsigned line;
	unsigned col;
};

struct variant {
	const rule_t **elements;
	const char *reducer;
	int reducer_tag;
};

struct rule {
	const char *name;
	const variant_t *variants;
};

struct token {
	int id;
	const char *first;
	const char *last;
	loc_t loc;
	union {
		void *ptr;
		int tag;
	};
};

extern const rule_t grammar_root;
