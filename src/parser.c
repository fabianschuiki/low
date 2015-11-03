/* Copyright (c) 2015 Fabian Schuiki */
#include "array.h"
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


int
parse (lexer_t *lex) {
	unsigned i;

	array_t stack;
	array_init(&stack, sizeof(parser_stack_t));
	parser_stack_t initial;
	bzero(&initial, sizeof(initial));
	array_add(&stack, &initial);

	while (stack.size > 0) {
		parser_stack_t *current = array_get(&stack, stack.size-1);
		const parser_state_t *state = parser_states + current->state;

		for (i = 0; i < state->num_actions; ++i)
			if (state->actions[i].token == lex->token)
				break;
		assert(i < state->num_actions && "syntax error");

		const parser_action_t *action = &state->actions[i];
		if (action->state_or_length < 0) {
			if (action->rule == 0) {
				break;
			}

			unsigned num_tokens = -action->state_or_length;
			// printf("reduce %d tokens with %p to %d\n", num_tokens, action->reducer, action->rule);

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
				action->reducer(&reduced, tokens, 0);
			} else {
				// unsigned length = reduced.last-reduced.first;
				// if (length > 35)
				// 	length = 35;
				// char *chr = strndup(reduced.first, length);
				// if (length == 35) {
				// 	chr[32] = '.';
				// 	chr[33] = '.';
				// 	chr[34] = '.';
				// }
				// printf("%s: %s\n", parser_token_names[reduced.id], chr);
				// free(chr);
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
			// printf("shift and goto %d\n", action->state_or_length);
			parser_stack_t *new_stack = array_add(&stack, 0);
			bzero(new_stack, sizeof(*new_stack));
			new_stack->state = action->state_or_length;
			new_stack->token.id = lex->token;
			new_stack->token.first = lex->base;
			new_stack->token.last = lex->ptr;
			lexer_next(lex);
		}

		// printf("  stack: [0]");
		// for (i = 1; i < stack.size; ++i) {
		// 	parser_stack_t *st = array_get(&stack, i);
		// 	printf(" %d [%d]", st->token, st->state);
		// }
		// printf("\n");
	}

	array_dispose(&stack);

	// array_t leads;
	// array_init(&leads, sizeof(lead_t));
	// lead_t root_lead = {&grammar_root, grammar_root.variants, 0, variant_token(grammar_root.variants,0), TKN_EOF};
	// array_add(&leads, &root_lead);
	// gather_leads(&leads, 0, 1);

	// state_stack_t stack = {0, {0, 0}};
	// parse_leads(lex, &leads, &stack);

#ifdef DEBUG_PARSER
	// printf("final token ");
	// print_token(&stack.token);
	// printf("\n");
#endif

	// array_dispose(&leads);

	// return stack.token.rule == &grammar_root ? 0 : 1;

	return 0;
}
