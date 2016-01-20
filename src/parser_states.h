/* Copyright (c) 2015-2016 Fabian Schuiki */
#pragma once
#include "grammar.h"

typedef struct parser_goto parser_goto_t;
typedef struct parser_action parser_action_t;
typedef struct parser_state parser_state_t;

struct parser_action {
	int token;
	int state_or_length;
	reduce_fn_t reducer;
	int reducer_tag;
	int rule; // after reduction
};

struct parser_goto {
	int rule; // on stack
	int state;
};

struct parser_state {
	const parser_action_t *actions;
	const parser_goto_t *gotos;
	int num_actions;
	int num_gotos;
};

extern const parser_state_t parser_states[];
extern const char *parser_token_names[];
