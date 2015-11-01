/* Copyright (c) 2015 Fabian Schuiki */
#include "array.h"
#include "grammar.h"
#include "parser.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>


typedef struct rule_chain rule_chain_t;
typedef struct state_stack state_stack_t;
typedef struct lead lead_t;


struct lead {
	const rule_t *rule;
	const variant_t *variant;
	unsigned position;
	token_t token;
	int terminal;
};

struct rule_chain {
	const rule_chain_t *prev;
	const rule_t *rule;
};

struct state_stack {
	state_stack_t *prev;
	token_t token;
};


static token_t
variant_token(const variant_t *self, unsigned index) {
	assert(self);
	const rule_t *e = self->elements[index];
	if ((size_t)e < MAX_TOKENS) {
		return (token_t){(int)(size_t)e, 0};
	} else {
		return (token_t){0, e};
	}
}


static int
equal_tokens (const token_t *t1, const token_t *t2) {
	return t1->token == t2->token && t1->rule == t2->rule;
}


static int
equal_leads (const lead_t *l1, const lead_t *l2) {
	return l1->rule == l2->rule &&
	       l1->variant == l2->variant &&
	       l1->position == l2->position &&
	       equal_tokens(&l1->token, &l2->token) &&
	       l1->terminal == l2->terminal;
}


#ifdef DEBUG_PARSER
static void
print_token (const token_t *token) {
	if (token->token && !token->rule)
		printf("%d", token->token);
	else if (token->rule && !token->token)
		printf("%s", token->rule->name);
	else if (!token->rule && !token->token)
		printf("<nil>");
	else
		printf("{%d, %s}", token->token, token->rule ? token->rule->name : "<none>");
}


static void
print_lead (const lead_t *lead) {
	printf("{%p, %s, %u, ", lead->variant, lead->rule->name, lead->position);
	print_token(&lead->token);
	printf(", %d}", lead->terminal);
}
#endif


void
gather_first (array_t *follow, const token_t *token, const rule_chain_t *chain) {
	assert(follow);
	assert(token);

	if (token->rule) {
		const rule_chain_t *check;
		for (check = chain; check; check = check->prev)
			if (check->rule == token->rule)
				return;
		rule_chain_t new_chain = {chain, token->rule};

		const variant_t *variant;
		for (variant = token->rule->variants; variant->elements; ++variant) {
			token_t t = variant_token(variant, 0);
			gather_first(follow, &t, &new_chain);
		}
	} else {
		assert(token->token && "no empty rules allowed");
		unsigned i;
		for (i = 0; i < follow->size; ++i)
			if (*(int*)array_get(follow, i) == token->token)
				break;
		if (i == follow->size)
			array_add(follow, &token->token);
	}
}


void
gather_leads (array_t *leads, unsigned index, unsigned length) {
	assert(leads);

	unsigned first_new = leads->size;
	unsigned i;
	for (i = index; i < index+length; ++i) {
		const lead_t *lead = array_get(leads, i);

		token_t tc = variant_token(lead->variant, lead->position);
		if (!tc.rule)
			continue;
		token_t tn = variant_token(lead->variant, lead->position+1);

		array_t follow;
		array_init(&follow, sizeof(int));
		if (tn.token == 0 && tn.rule == 0) {
			array_add(&follow, &lead->terminal);
		} else {
			gather_first(&follow, &tn, 0);
		}

		const variant_t *variant;
		for (variant = tc.rule->variants; variant->elements; ++variant) {

			unsigned n;
			for (n = 0; n < follow.size; ++n) {
				lead_t nl = {
					tc.rule,
					variant,
					0,
					variant_token(variant, 0),
					*(int*)array_get(&follow, n)
				};

				unsigned m;
				for (m = 0; m < leads->size; ++m) {
					const lead_t *lead = array_get(leads, m);
					if (equal_leads(lead, &nl))
						break;
				}

				if (m == leads->size) {
					array_add(leads, &nl);
				}
			}
		}

		array_dispose(&follow);
	}

	if (first_new < leads->size)
		gather_leads(leads, first_new, leads->size - first_new);
}


static int
parse_leads (lexer_t *lex, const array_t *leads, state_stack_t *stack) {

	unsigned i;
#ifdef DEBUG_PARSER
	printf("parse:\n");
	for (i = 0; i < leads->size; ++i) {
		printf("  lead %d ", i);
		print_lead(array_get(leads, i));
		printf("\n");
	}

	printf("  token %d\n", lex->token);

	printf("  stack:\n");
	state_stack_t *trace;
	for (trace = stack; trace; trace = trace->prev) {
		printf("    ");
		print_token(&trace->token);
		printf("\n");
	}
#endif

	array_t shift_leads, reduce_leads;
	array_init(&shift_leads, sizeof(lead_t));
	array_init(&reduce_leads, sizeof(lead_t));

	for (i = 0; i < leads->size; ++i) {
		lead_t *lead = array_get(leads, i);

		if (lead->token.token == lex->token) {
			lead_t nl = *lead;
			++nl.position;
			nl.token = variant_token(nl.variant, nl.position);
			array_add(&shift_leads, &nl);
		} else if (lead->token.token == 0 && lead->token.rule == 0 && lead->terminal == lex->token) {
			array_add(&reduce_leads, lead);
		}
	}

#ifdef DEBUG_PARSER
	printf("  options:\n");
	for (i = 0; i < shift_leads.size; ++i) {
		printf("    shift %d ", i);
		print_lead(array_get(&shift_leads, i));
		printf("\n");
	}
	for (i = 0; i < reduce_leads.size; ++i) {
		printf("    reduce %d ", i);
		print_lead(array_get(&reduce_leads, i));
		printf("\n");
	}
#endif

	assert((shift_leads.size > 0 || reduce_leads.size > 0) && "syntax error");
	assert((shift_leads.size > 0) != (reduce_leads.size > 0) && "shift-reduce conflict");
	assert(reduce_leads.size <= 1 && "reduce-reduce conflict");

	int result = 0;
	if (shift_leads.size > 0) {
		gather_leads(&shift_leads, 0, shift_leads.size);

		state_stack_t new_stack = {stack, {lex->token, 0,
			.lex.first = lex->base,
			.lex.last = lex->ptr,
		}};
		lexer_next(lex);
		int backoff = parse_leads(lex, &shift_leads, &new_stack);

		if (backoff > 0) {
			result = backoff-1;
			goto wrapup;
		} else if (backoff < 0) {
			result = backoff;
			goto wrapup;
		}

		array_t new_leads;
		array_init(&new_leads, sizeof(lead_t));
		do {
		#ifdef DEBUG_PARSER
			printf("  post-reduction (");
			print_token(&new_stack.token);
			printf("):\n");

			state_stack_t *trace;
			for (trace = &new_stack; trace; trace = trace->prev) {
				printf("    ");
				print_token(&trace->token);
				printf("\n");
			}
		#endif

			for (i = 0; i < leads->size; ++i) {
				lead_t *lead = array_get(leads, i);
				if (lead->token.rule == new_stack.token.rule) {
				#ifdef DEBUG_PARSER
					printf("    goto %d ", i);
					print_lead(lead);
					printf("\n");
				#endif

					lead_t nl = *lead;
					++nl.position;
					nl.token = variant_token(nl.variant, nl.position);
					array_add(&new_leads, &nl);
				}
			}

			if (new_leads.size == 0) {
				if (new_stack.token.rule == &grammar_root && lex->token == TKN_EOF) {
					stack->token = new_stack.token;
					backoff = 0;
					result = 0;
					break;
				} else {
					assert(0 && "syntax error");
				}
			} else {
				gather_leads(&new_leads, 0, new_leads.size);
				backoff = parse_leads(lex, &new_leads, &new_stack);
				if (backoff > 0) {
					result = backoff-1;
					break;
				} else if (backoff < 0) {
					result = backoff;
					break;
				}
			}

			array_clear(&new_leads);
		} while (backoff == 0);
		array_dispose(&new_leads);

	} else if (reduce_leads.size > 0) {
		const lead_t *lead = reduce_leads.items;

		unsigned num_tokens = lead->position;
		token_t tokens[num_tokens];
		state_stack_t *trace, *tail = 0;
		for (trace = stack, i = 0; i < num_tokens; ++i, trace = trace->prev) {
			assert(trace);
			tokens[num_tokens-i-1] = trace->token;
			tail = trace;
		}

	#ifdef DEBUG_PARSER
		printf("  reduce using %s:", lead->rule->name);
		for (i = 0; i < num_tokens; ++i) {
			printf(" ");
			print_token(&tokens[i]);
		}
		printf("\n");
	#else
		printf("%s\n", lead->rule->name);
	#endif

		tail->token.rule = lead->rule;
		tail->token.token = 0;
		reduce_fn_t reducer = lead->variant->reducer;
		if (reducer) {
			reducer(tokens, num_tokens, lead->variant->reducer_arg);
		}

		result = num_tokens-1;
	}

wrapup:
	array_dispose(&shift_leads);
	array_dispose(&reduce_leads);
	return result;
}


int
parse (lexer_t *lex) {

	array_t leads;
	array_init(&leads, sizeof(lead_t));
	lead_t root_lead = {&grammar_root, grammar_root.variants, 0, variant_token(grammar_root.variants,0), TKN_EOF};
	array_add(&leads, &root_lead);
	gather_leads(&leads, 0, 1);

	state_stack_t stack = {0, {0, 0}};
	parse_leads(lex, &leads, &stack);

#ifdef DEBUG_PARSER
	printf("final token ");
	print_token(&stack.token);
	printf("\n");
#endif

	array_dispose(&leads);

	return stack.token.rule == &grammar_root ? 0 : 1;
}
