/* Copyright (c) 2015-2016 Fabian Schuiki */
#include "ast.h"
#include "parser.h"
#include "parser_states.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


typedef struct parser_stack parser_stack_t;

struct parser_stack {
	int state;
	token_t token;
};


array_t *
parse (lexer_t *lex) {
	unsigned i;

	array_t stack;
	array_init(&stack, sizeof(parser_stack_t));
	parser_stack_t initial;
	bzero(&initial, sizeof(initial));
	array_add(&stack, &initial);

	array_t *result = 0;
	while (stack.size > 0) {
		parser_stack_t *current = array_get(&stack, stack.size-1);
		const parser_state_t *state = parser_states + current->state;

		for (i = 0; i < state->num_actions; ++i)
			if (state->actions[i].token == lex->token)
				break;

		if (i >= state->num_actions) {
			char *msg = strdup("syntax error, expected");
			for (i = 0; i < state->num_actions; ++i) {
				char *glue;
				if (i == 0)
					glue = " ";
				else if (i == state->num_actions-1)
					glue = ", or ";
				else
					glue = ", ";
				char *nmsg;
				asprintf(&nmsg, "%s%s\"%s\"", msg, glue, token_names[state->actions[i].token]);
				free(msg);
				msg = nmsg;
			}
			derror(&lex->loc, "%s\n", msg);
			free(msg);
		}

		const parser_action_t *action = &state->actions[i];
		if (action->state_or_length < 0) {
			if (action->rule == 0) {
				result = current->token.ptr;
				break;
			}

			unsigned num_tokens = -action->state_or_length;

			parser_stack_t *target = array_get(&stack, stack.size-num_tokens);
			parser_stack_t *base = array_get(&stack, stack.size-num_tokens-1);
			const parser_state_t *base_state = parser_states + base->state;

			token_t reduced = target->token;
			reduced.id = action->rule;
			reduced.last = current->token.last;

			if (action->reducer) {
				token_t tokens[num_tokens];
				for (i = 0; i < num_tokens; ++i)
					tokens[i] = (target+i)->token;
				action->reducer(&reduced, tokens, action->reducer_tag);
			}

			target->token = reduced;

			for (i = 0; i < base_state->num_gotos; ++i) {
				if (base_state->gotos[i].rule == action->rule) {
					target->state = base_state->gotos[i].state;
					break;
				}
			}

			array_resize(&stack, stack.size-num_tokens+1);
		} else {
			parser_stack_t *new_stack = array_add(&stack, 0);
			bzero(new_stack, sizeof(*new_stack));
			new_stack->state = action->state_or_length;
			new_stack->token.id = lex->token;
			new_stack->token.first = lex->base;
			new_stack->token.last = lex->ptr;
			new_stack->token.loc = lex->loc;
			lexer_next(lex);
		}
	}

	array_dispose(&stack);

	return result;
}
