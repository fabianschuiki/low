/* Copyright (c) 2015 Fabian Schuiki, Thomas Richner */
#include "array.h"
#include "ast.h"
#include "grammar.h"
#include "lexer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REDUCER(name) void reduce_##name(token_t *out, const token_t *in, int tag)


// --- primary_expr ------------------------------------------------------------

REDUCER(primary_expr_ident) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_IDENT_EXPR;
	e->loc = in->loc;
	e->ident = strndup(in->first, in->last - in->first);
	out->ptr = e;
}

REDUCER(primary_expr_string) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_STRING_LITERAL_EXPR;
	e->loc = in->loc;
	unsigned length = in->last-in->first-2;
	const char *data = in->first+1;
	char *str = malloc(length+1);
	char *ptr = str;
	unsigned i;
	for (i = 0; i < length; ++i) {
		if (data[i] == '\\') {
			++i;
			switch (data[i]) {
				case '"': *ptr++ = '"'; break;
				case 't': *ptr++ = '\t'; break;
				case 'n': *ptr++ = '\n'; break;
				case 'r': *ptr++ = '\r'; break;
				default:
					fprintf(stderr, "unknown escape sequence \\%c\n", data[i]);
					abort();
					break;
			}
		} else {
			*ptr++ = data[i];
		}
	}
	*ptr = 0;
	e->string_literal = str;
	// e->string_literal = strndup(in->first+1, in->last-in->first-2);
	out->ptr = e;
}


static int
is_number_dec (int c) {
	return (c >= '0' && c <= '9');
}

typedef struct radix_prefix {
	int radix;
	int prefix;
} radix_prefix_t;

static radix_prefix_t
determine_radix(const char *ptr, const char *end){
	const char* start = ptr;

	radix_prefix_t pfx = {
		.radix = 10,
		.prefix = 0
	};

	if(!is_number_dec(*ptr)){
		return pfx;
	}
	ptr++;

	if(ptr==end){
		return pfx;
	}

	if(is_number_dec(*ptr)){
		ptr++;
		if(ptr==end){
			return pfx;
		}
	}

	if(*ptr=='x'){
		char* aradix = strndup(start,ptr-start);
		pfx.prefix = ptr-start + 1;
		int radix = atoi(aradix);
		if(radix==0){ // 0x case
			radix = 16;
		}
		pfx.radix = radix;
		return pfx;
	}
	return pfx;
}

static char*
extract_literal(const char* str,unsigned int n){
	char* literal = calloc(n,sizeof(char));
	assert(literal);

	int j=0;
	for(;n>0;n--){
		if(*str!='\''){
			literal[j] = *str;
			j++;
		}
		str++;
	}
	return literal;
}

REDUCER(primary_expr_number) {
	radix_prefix_t pfx = determine_radix(in->first,in->last);

	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_NUMBER_LITERAL_EXPR;
	e->loc = in->loc;
	e->number_literal.radix = pfx.radix;

	const char* first = in->first + pfx.prefix;
	e->number_literal.literal = extract_literal(first, in->last-first);
	out->ptr = e;
}

REDUCER(primary_expr_paren) {
	out->ptr = in[1].ptr;
}


// --- postfix_expr ------------------------------------------------------------

REDUCER(postfix_expr_index) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_INDEX_ACCESS_EXPR;
	e->loc = in[1].loc;
	e->index_access.target = in[0].ptr;
	e->index_access.index = in[2].ptr;
	out->ptr = e;
}

REDUCER(postfix_expr_call) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_CALL_EXPR;
	e->loc = in[1].loc;
	e->call.target = in[0].ptr;
	if (tag == 1) {
		array_t *args = in[2].ptr;
		assert(args);
		array_shrink(args);
		e->call.num_args = args->size;
		e->call.args = args->items;
		free(args);
	}
	out->ptr = e;
}

REDUCER(postfix_expr_member) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_MEMBER_ACCESS_EXPR;
	e->loc = in[1].loc;
	e->member_access.target = in[0].ptr;
	e->member_access.name = strndup(in[2].first, in[2].last-in[2].first);
	out->ptr = e;
}

REDUCER(postfix_expr_incdec) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_INCDEC_EXPR;
	e->loc = in[1].loc;
	e->incdec_op.order = AST_POST;
	if (tag == 0) {
		e->incdec_op.direction = AST_INC;
	} else {
		e->incdec_op.direction = AST_DEC;
	}
	e->incdec_op.target = in[0].ptr;
	out->ptr = e;
}

REDUCER(argument_expr_list) {
	if (tag == 0) {
		array_t *args = malloc(sizeof(array_t));
		array_init(args, sizeof(expr_t));
		array_add(args, in[0].ptr);
		free(in[0].ptr);
		out->ptr = args;
	} else {
		array_add(in[0].ptr, in[2].ptr);
		free(in[2].ptr);
	}
}

REDUCER(postfix_expr_initializer) {
	assert(0 && "not implemented");
}


// --- unary_expr --------------------------------------------------------------

REDUCER(unary_expr_incdec) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_INCDEC_EXPR;
	e->loc = in[0].loc;
	e->incdec_op.order = AST_PRE;
	if (tag == 0) {
		e->incdec_op.direction = AST_INC;
	} else {
		e->incdec_op.direction = AST_DEC;
	}
	e->incdec_op.target = in[1].ptr;
	out->ptr = e;
}

REDUCER(unary_expr_op) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_UNARY_EXPR;
	e->loc = in[0].loc;
	e->unary_op.target = in[1].ptr;
	e->unary_op.op = in[0].tag;
	out->ptr = e;
}

REDUCER(unary_op) {
	switch (in->id) {
		case TKN_BITWISE_AND: out->tag = AST_ADDRESS; break;
		case TKN_MUL_OP: out->tag = AST_DEREF; break;
		case TKN_ADD_OP: out->tag = AST_POSITIVE; break;
		case TKN_SUB_OP: out->tag = AST_NEGATIVE; break;
		case TKN_BITWISE_NOT: out->tag = AST_BITWISE_NOT; break;
		case TKN_NOT_OP: out->tag = AST_NOT; break;
		default:
			fprintf(stderr, "%s.%d: unary operator %d not handled\n", __FILE__, __LINE__, in->id);
			abort();
			break;
	}
}

REDUCER(unary_expr_sizeof) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_SIZEOF_EXPR;
	e->loc = in[0].loc;
	if (tag == 0) {
		e->sizeof_op.mode = AST_EXPR_SIZEOF;
		e->sizeof_op.expr = in[1].ptr;
	} else {
		e->sizeof_op.mode = AST_TYPE_SIZEOF;
		e->sizeof_op.type = *(type_t*)in[2].ptr;
		free(in[2].ptr);
	}
	out->ptr = e;
}

REDUCER(builtin_func_new) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_NEW_BUILTIN;
	e->loc = in[0].loc;

	e->newe.type = *(type_t*)in[2].ptr;		/* type to alloc */
	free(in[2].ptr);
	out->ptr = e;
}

REDUCER(builtin_func_free) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_FREE_BUILTIN;
	e->loc = in[0].loc;
	e->free.expr = in[2].ptr;
	out->ptr = e;
}

REDUCER(builtin_func_make) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_MAKE_BUILTIN;
	e->loc = in[0].loc;
	e->make.type = *(type_t*)in[2].ptr;
	e->make.expr = in[4].ptr;
	out->ptr = e;
}



REDUCER(builtin_func_lencap) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->loc = in[0].loc;
	e->lencap.expr = in[2].ptr;
	e->lencap.kind = tag;

	e->kind = AST_LENCAP_BUILTIN;
	out->ptr = e;
}

// --- cast_expr ---------------------------------------------------------------

REDUCER(cast_expr) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_CAST_EXPR;
	e->loc = in[0].loc;
	e->cast.target = in[4].ptr;
	e->cast.type = *(type_t*)in[2].ptr;
	free(in[2].ptr);
	out->ptr = e;
}

REDUCER(cast_expr2) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_CAST_EXPR;
	e->loc = in[0].loc;
	e->cast.target = in[3].ptr;
	e->cast.type = *(type_t*)in[1].ptr;
	free(in[1].ptr);
	out->ptr = e;
}


// --- multiplicative_expr -----------------------------------------------------

REDUCER(binary_expr) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_BINARY_EXPR;
	e->loc = in[1].loc;
	e->binary_op.lhs = in[0].ptr;
	e->binary_op.rhs = in[2].ptr;
	switch (in[1].id) {
		case TKN_MUL_OP:      e->binary_op.op = AST_MUL; break;
		case TKN_DIV_OP:      e->binary_op.op = AST_DIV; break;
		case TKN_MOD_OP:      e->binary_op.op = AST_MOD; break;
		case TKN_ADD_OP:      e->binary_op.op = AST_ADD; break;
		case TKN_SUB_OP:      e->binary_op.op = AST_SUB; break;
		case TKN_LEFT_OP:     e->binary_op.op = AST_LEFT; break;
		case TKN_RIGHT_OP:    e->binary_op.op = AST_RIGHT; break;
		case TKN_LT_OP:       e->binary_op.op = AST_LT; break;
		case TKN_GT_OP:       e->binary_op.op = AST_GT; break;
		case TKN_LE_OP:       e->binary_op.op = AST_LE; break;
		case TKN_GE_OP:       e->binary_op.op = AST_GE; break;
		case TKN_EQ_OP:       e->binary_op.op = AST_EQ; break;
		case TKN_NE_OP:       e->binary_op.op = AST_NE; break;
		case TKN_BITWISE_AND: e->binary_op.op = AST_BITWISE_AND; break;
		case TKN_BITWISE_XOR: e->binary_op.op = AST_BITWISE_XOR; break;
		case TKN_BITWISE_OR:  e->binary_op.op = AST_BITWISE_OR; break;
		case TKN_AND_OP:      e->binary_op.op = AST_AND; break;
		case TKN_OR_OP:       e->binary_op.op = AST_OR; break;
		default:
			fprintf(stderr, "%s.%d: binary operator %d not handled\n", __FILE__, __LINE__, in->id);
			abort();
			break;
	}
	out->ptr = e;
}


// --- conditional_expr --------------------------------------------------------

REDUCER(conditional_expr) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_CONDITIONAL_EXPR;
	e->loc = in[1].loc;
	e->conditional.condition = in[0].ptr;
	e->conditional.true_expr = in[2].ptr;
	e->conditional.false_expr = in[4].ptr;
	out->ptr = e;
}


// --- assignment_expr ---------------------------------------------------------

REDUCER(assignment_expr) {
	expr_t *e = malloc(sizeof(expr_t));
	bzero(e, sizeof(*e));
	e->kind = AST_ASSIGNMENT_EXPR;
	e->loc = in[1].loc;
	e->assignment.target = in[0].ptr;
	e->assignment.expr = in[2].ptr;
	e->assignment.op = in[1].tag;
	out->ptr = e;
}

REDUCER(assignment_op) {
	switch (in->id) {
		case TKN_ASSIGN: out->tag = AST_ASSIGN; break;
		case TKN_ADD_ASSIGN: out->tag = AST_ADD_ASSIGN; break;
		case TKN_AND_ASSIGN: out->tag = AST_AND_ASSIGN; break;
		case TKN_DIV_ASSIGN: out->tag = AST_DIV_ASSIGN; break;
		case TKN_LEFT_ASSIGN: out->tag = AST_LEFT_ASSIGN; break;
		case TKN_MOD_ASSIGN: out->tag = AST_MOD_ASSIGN; break;
		case TKN_MUL_ASSIGN: out->tag = AST_MUL_ASSIGN; break;
		case TKN_OR_ASSIGN: out->tag = AST_OR_ASSIGN; break;
		case TKN_RIGHT_ASSIGN: out->tag = AST_RIGHT_ASSIGN; break;
		case TKN_SUB_ASSIGN: out->tag = AST_SUB_ASSIGN; break;
		case TKN_XOR_ASSIGN: out->tag = AST_XOR_ASSIGN; break;
		default:
			fprintf(stderr, "%s.%d: assignment operator %d not handled\n", __FILE__, __LINE__, in->id);
			abort();
			break;
	}
}


// --- expr --------------------------------------------------------------------

REDUCER(expr_comma) {
	expr_t *e = in->ptr;
	if (e->kind != AST_COMMA_EXPR) {
		expr_t *ce = malloc(sizeof(expr_t));
		bzero(ce, sizeof(*ce));
		ce->kind = AST_COMMA_EXPR;
		ce->loc = e->loc;
		ce->comma.num_exprs = 2;
		ce->comma.exprs = malloc(2 * sizeof(expr_t));
		ce->comma.exprs[0] = *e;
		ce->comma.exprs[1] = *(expr_t*)in[2].ptr;
		out->ptr = ce;
		free(e);
		free(in[2].ptr);
	} else {
		e->comma.exprs = realloc(e->comma.exprs, (e->comma.num_exprs+1)*sizeof(expr_t));
		e->comma.exprs[e->comma.num_exprs] = *(expr_t*)in[2].ptr;
		++e->comma.num_exprs;
		free(in[2].ptr);
	}
}


// --- expr_stmt ---------------------------------------------------------------

REDUCER(expr_stmt) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_EXPR_STMT;
	s->expr = in->ptr;
	s->loc = s->expr->loc;
	out->ptr = s;
}


// --- compound_stmt -----------------------------------------------------------

REDUCER(compound_stmt) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_COMPOUND_STMT;
	s->loc = in[0].loc;
	array_t *items = in[1].ptr;
	array_shrink(items);
	s->compound.num_items = items->size;
	s->compound.items = items->items;
	free(items);
	out->ptr = s;
}

REDUCER(block_item_list) {
	if (tag == 0) {
		array_t *items = malloc(sizeof(array_t));
		array_init(items, sizeof(block_item_t));
		if (in[0].ptr) {
			array_add(items, in[0].ptr);
			free(in[0].ptr);
		}
		out->ptr = items;
	} else {
		if (in[1].ptr) {
			array_add(in[0].ptr, in[1].ptr);
			free(in[1].ptr);
		}
	}
}

REDUCER(block_item_decl) {
	block_item_t *b = malloc(sizeof(block_item_t));
	bzero(b, sizeof(*b));
	b->kind = AST_DECL_BLOCK_ITEM;
	b->decl = in->ptr;
	// b->loc = b->decl->loc;
	out->ptr = b;
}

REDUCER(block_item_stmt) {
	block_item_t *b = malloc(sizeof(block_item_t));
	bzero(b, sizeof(*b));
	b->kind = AST_STMT_BLOCK_ITEM;
	b->stmt = in->ptr;
	// b->loc = b->stmt->loc;
	out->ptr = b;
}


// --- selection_stmt ----------------------------------------------------------

REDUCER(selection_stmt_if) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_IF_STMT;
	s->loc = in[0].loc;
	s->selection.condition = in[2].ptr;
	s->selection.stmt = in[4].ptr;
	if (tag == 1)
		s->selection.else_stmt = in[6].ptr;
	out->ptr = s;
}

REDUCER(selection_stmt_switch) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_SWITCH_STMT;
	s->loc = in[0].loc;
	s->selection.condition = in[2].ptr;
	s->selection.stmt = in[4].ptr;
	out->ptr = s;
}


// --- iteration_stmt ----------------------------------------------------------

REDUCER(iteration_stmt_while) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_WHILE_STMT;
	s->loc = in[0].loc;
	s->iteration.condition = in[2].ptr;
	s->iteration.stmt = in[4].ptr;
	out->ptr = s;
}

REDUCER(iteration_stmt_do) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_DO_STMT;
	s->loc = in[0].loc;
	s->iteration.stmt = in[1].ptr;
	s->iteration.condition = in[4].ptr;
	out->ptr = s;
}

REDUCER(iteration_stmt_for) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_FOR_STMT;
	s->loc = in[0].loc;
	if (in[2].ptr) {
		s->iteration.initial = ((stmt_t*)in[2].ptr)->expr;
		free(in[2].ptr);
	}
	if (in[3].ptr) {
		s->iteration.condition = ((stmt_t*)in[3].ptr)->expr;
		free(in[3].ptr);
	}
	if (tag == 0) {
		s->iteration.step = in[4].ptr;
		s->iteration.stmt = in[6].ptr;
	} else {
		s->iteration.stmt = in[5].ptr;
	}
	out->ptr = s;
}

REDUCER(iteration_stmt_for2) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_FOR_STMT;
	s->loc = in[0].loc;

	if (in[1].ptr) {
		s->iteration.initial = ((stmt_t*)in[1].ptr)->expr;
		free(in[1].ptr);
	}
	if (in[2].ptr) {
		s->iteration.condition = ((stmt_t*)in[2].ptr)->expr;
		free(in[2].ptr);
	}
	if (tag == 0) {
		s->iteration.step = in[3].ptr;
		s->iteration.stmt = in[5].ptr;
	} else {
		s->iteration.stmt = in[4].ptr;
	}
	out->ptr = s;
}

REDUCER(iteration_stmt_for_cond){
	fprintf(stderr,"while like for loop not supported yet :'(\n");
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	//s->kind = AST_FOR_COND_STMT;
	s->loc = in[0].loc;

	if (in[1].ptr) {
		s->iteration.initial = ((stmt_t*)in[1].ptr)->expr;
		free(in[2].ptr);
	}else{
		//s->iteration.initial.kind = AST_CONDITIONAL_EXPR;
	}

}

// --- jump_stmt ---------------------------------------------------------------

REDUCER(jump_stmt_goto) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_GOTO_STMT;
	s->name = strndup(in[1].first, in[1].last-in[1].first);
	out->ptr = s;
}

REDUCER(jump_stmt_continue) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_CONTINUE_STMT;
	s->loc = in->loc;
	out->ptr = s;
}

REDUCER(jump_stmt_break) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_BREAK_STMT;
	s->loc = in->loc;
	out->ptr = s;
}

REDUCER(jump_stmt_return) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_RETURN_STMT;
	s->loc = in->loc;
	if (tag == 1)
		s->expr = in[1].ptr;
	out->ptr = s;
}


// --- labeled_stmt ------------------------------------------------------------

REDUCER(labeled_stmt_name) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_LABEL_STMT;
	s->label.name = strndup(in[0].first, in[0].last-in[0].first);
	s->label.stmt = in[2].ptr;
	out->ptr = s;
}

REDUCER(labeled_stmt_case) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_CASE_STMT;
	s->label.expr = in[1].ptr;
	s->label.stmt = in[3].ptr;
	out->ptr = s;
}

REDUCER(labeled_stmt_default) {
	stmt_t *s = malloc(sizeof(stmt_t));
	bzero(s, sizeof(*s));
	s->kind = AST_DEFAULT_STMT;
	s->label.stmt = in[2].ptr;
	out->ptr = s;
}


// --- decl --------------------------------------------------------------------

REDUCER(variable_decl) {
	decl_t *d = malloc(sizeof(decl_t));
	bzero(d, sizeof(*d));
	d->kind = AST_VARIABLE_DECL;
	unsigned p = 1;
	if (tag == 0 || tag == 1) {
		d->variable.type = *(type_t*)in[p].ptr;
		free(in[p].ptr);
		++p;
	}
	d->loc = in[p].loc;
	d->variable.name = strndup(in[p].first, in[p].last-in[p].first);
	p += 2;
	if (tag == 1 || tag == 2)
		d->variable.initial = in[p].ptr;
	out->ptr = d;
}

REDUCER(variable_decl2) {
	decl_t *d = malloc(sizeof(decl_t));
	bzero(d, sizeof(*d));
	d->kind = AST_VARIABLE_DECL;
	unsigned p = 0;
	d->loc = in[p].loc;
	d->variable.name = strndup(in[p].first, in[p].last-in[p].first);
	++p;
	if (tag == 0 || tag == 1) {
		++p;
		d->variable.type = *(type_t*)in[p].ptr;
		free(in[p].ptr);
		++p;
	}
	++p;
	if (tag == 1 || tag == 2)
		d->variable.initial = in[p].ptr;
	out->ptr = d;
}

REDUCER(const_decl) {
	decl_t *d = malloc(sizeof(decl_t));
	bzero(d, sizeof(*d));
	d->kind = AST_CONST_DECL;
	unsigned i = 1;
	d->loc = in[i].loc;
	d->cons.name = strndup(in[i].first, in[i].last-in[i].first);
	++i;
	if (tag == 1) {
		d->cons.type = in[i].ptr;
		++i;
	}
	++i;
	d->cons.value = *(expr_t*)in[i].ptr;
	free(in[i].ptr);
	++i;
	out->ptr = d;
}


// --- type --------------------------------------------------------------------

REDUCER(type_void) {
	type_t *t = malloc(sizeof(type_t));
	bzero(t, sizeof(*t));
	t->kind = AST_VOID_TYPE;
	out->ptr = t;
}

REDUCER(type_name) {
	type_t *t = malloc(sizeof(type_t));
	bzero(t, sizeof(*t));
	const char *name = in[0].first;
	unsigned len = in[0].last-in[0].first;
	if (len >= 3 && strncmp(name, "int", 3) == 0) {
		t->kind = AST_INTEGER_TYPE;
		if (len > 3) {
			char suffix[len-3+1];
			strncpy(suffix, name+3, len-3);
			suffix[len-3] = 0;
			t->width = atoi(suffix);
		} else {
			t->width = 32;
		}
	} else if (len >= 5 && strncmp(name, "float", 5) == 0) {
		t->kind = AST_FLOAT_TYPE;
		if (len > 5) {
			char suffix[len-5+1];
			strncpy(suffix, name+5, len-5);
			suffix[len-5] = 0;
			t->width = atoi(suffix);
		} else {
			t->width = 32;
		}
	} else if (len == 4 && strncmp(name, "bool", 4) == 0) {
		t->kind = AST_BOOLEAN_TYPE;
	} else {
		t->kind = AST_NAMED_TYPE;
		t->name = strndup(name,len);
	}
	out->ptr = t;
}

REDUCER(type_pointer) {
	type_t *t = in[0].ptr;
	++t->pointer;
}

REDUCER(type_struct) {
	type_t *t = malloc(sizeof(type_t));
	bzero(t, sizeof(*t));
	t->kind = AST_STRUCT_TYPE;
	array_t *a = in[2].ptr;
	array_shrink(a);
	t->strct.num_members = a->size;
	t->strct.members = a->items;
	free(a);
	out->ptr = t;
}

REDUCER(struct_member_list) {
	if (tag == 0) {
		array_t *a = malloc(sizeof(array_t));
		array_init(a, sizeof(struct_member_t));
		array_add(a, in[0].ptr);
		free(in[0].ptr);
		out->ptr = a;
	} else {
		array_add(in[0].ptr, in[1].ptr);
		free(in[1].ptr);
	}
}

REDUCER(struct_member) {
	struct_member_t *m = malloc(sizeof(struct_member_t));
	bzero(m, sizeof(*m));
	m->type = in[0].ptr;
	m->name = strndup(in[1].first, in[1].last-in[1].first);
	out->ptr = m;
}

REDUCER(struct_member2) {
	struct_member_t *m = malloc(sizeof(struct_member_t));
	bzero(m, sizeof(*m));
	m->name = strndup(in[0].first, in[0].last-in[0].first);
	m->type = in[2].ptr;
	out->ptr = m;
}

REDUCER(type_array) {
	type_t *t = malloc(sizeof(type_t));
	bzero(t, sizeof(*t));
	t->kind = AST_ARRAY_TYPE;
	t->array.type = in[3].ptr;
	t->array.length = atoi(in[1].first);
	out->ptr = t;
}

REDUCER(type_slice) {
	type_t *t = malloc(sizeof(type_t));
	bzero(t, sizeof(*t));
	t->kind = AST_SLICE_TYPE;
	t->slice.type = in[2].ptr;
	out->ptr = t;
}

REDUCER(type_func) {
	type_t *t = malloc(sizeof(type_t));
	bzero(t, sizeof(*t));
	t->kind = AST_FUNC_TYPE;
	t->pointer = 1;
	if (tag == 0) {
		t->func.return_type = in[3].ptr;
	} else {
		array_t *p = in[2].ptr;
		array_shrink(p);
		t->func.num_args = p->size;
		t->func.args = p->items;
		free(p);
		t->func.return_type = in[4].ptr;
	}
	out->ptr = t;
}

REDUCER(func_type_args) {
	if (tag == 0) {
		array_t *a = malloc(sizeof(array_t));
		array_init(a, sizeof(type_t));
		array_add(a, in[0].ptr);
		free(in[0].ptr);
		out->ptr = a;
	} else {
		array_add(in[0].ptr, in[2].ptr);
		free(in[2].ptr);
	}
}

REDUCER(type_interface) {
	type_t *t = malloc(sizeof(type_t));
	bzero(t, sizeof(*t));
	t->kind = AST_INTERFACE_TYPE;
	if (tag == 1) {
		array_t *p = in[2].ptr;
		array_shrink(p);
		t->interface.num_members = p->size;
		t->interface.members = p->items;
		free(p);
	}
	out->ptr = t;
}

REDUCER(interface_member_list) {
	if (tag == 0) {
		array_t *a = malloc(sizeof(array_t));
		array_init(a, sizeof(interface_member_t));
		array_add(a, in[0].ptr);
		free(in[0].ptr);
		out->ptr = a;
	} else {
		array_add(in[0].ptr, in[1].ptr);
		free(in[1].ptr);
	}
}

REDUCER(interface_member_field) {
	interface_member_t *m = malloc(sizeof(interface_member_t));
	bzero(m, sizeof(*m));
	m->name = strndup(in[0].first, in[0].last-in[0].first);
	m->type = in[2].ptr;
	out->ptr = m;
}


// --- unit --------------------------------------------------------------------

REDUCER(import_unit) {
	unit_t *u = malloc(sizeof(unit_t));
	bzero(u, sizeof(*u));
	u->kind = AST_IMPORT_UNIT;
	u->loc = in[1].loc;
	u->import_name = strndup(in[1].first+1, in[1].last-in[1].first-2);
	out->ptr = u;
}

REDUCER(decl_unit) {
	unit_t *u = malloc(sizeof(unit_t));
	bzero(u, sizeof(*u));
	u->kind = AST_DECL_UNIT;
	u->decl = in->ptr;
	u->loc = u->decl->loc;
	out->ptr = u;
}

REDUCER(func_unit) {
	unit_t *u = in->ptr;
	if (tag == 1)
		u->func.body = in[1].ptr;
	out->ptr = u;
}

REDUCER(func_unit_decl) {
	unit_t *u = malloc(sizeof(unit_t));
	bzero(u, sizeof(*u));
	u->kind = AST_FUNC_UNIT;
	u->loc = in[1].loc;
	u->func.return_type = *(type_t*)in[0].ptr;
	free(in[0].ptr);
	u->func.name = strndup(in[1].first, in[1].last-in[1].first);
	u->func.variadic = (tag == 1 || tag == 3);
	if (tag == 2 || tag == 3) {
		array_t *p = in[3].ptr;
		array_shrink(p);
		u->func.num_params = p->size;
		u->func.params = p->items;
		free(p);
	}
	out->ptr = u;
}

REDUCER(func_unit_decl2) {
	unit_t *u = malloc(sizeof(unit_t));
	bzero(u, sizeof(*u));
	u->kind = AST_FUNC_UNIT;
	u->loc = in[1].loc;
	u->func.name = strndup(in[1].first, in[1].last-in[1].first);
	u->func.variadic = (tag == 1 || tag == 3);
	if (tag == 2 || tag == 3) {
		array_t *p = in[3].ptr;
		array_shrink(p);
		u->func.num_params = p->size;
		u->func.params = p->items;
		free(p);
	}
	unsigned i = 4;
	if (tag == 1 || tag == 2) i = 5;
	if (tag == 3) i = 7;
	u->func.return_type = *(type_t*)in[i].ptr;
	free(in[i].ptr);
	out->ptr = u;
}

REDUCER(parameter_list) {
	if (tag == 0) {
		array_t *a = malloc(sizeof(array_t));
		array_init(a, sizeof(func_param_t));
		array_add(a, in[0].ptr);
		free(in[0].ptr);
		out->ptr = a;
	} else {
		array_add(in[0].ptr, in[2].ptr);
		free(in[2].ptr);
	}
}

REDUCER(parameter) {
	func_param_t *p = malloc(sizeof(func_param_t));
	bzero(p, sizeof(*p));
	p->type = *(type_t*)in[0].ptr;
	free(in[0].ptr);
	if (tag == 1)
		p->name = strndup(in[1].first, in[1].last-in[1].first);
	out->ptr = p;
}

REDUCER(type_unit) {
	unit_t *u = malloc(sizeof(unit_t));
	bzero(u, sizeof(*u));
	u->kind = AST_TYPE_UNIT;
	u->loc = in[1].loc;
	u->type.type = *(type_t*)in[3].ptr;
	free(in[3].ptr);
	u->type.name = strndup(in[1].first, in[1].last-in[1].first);
	out->ptr = u;
}

REDUCER(unit_list) {
	if (tag == 0) {
		array_t *a = malloc(sizeof(array_t));
		array_init(a, sizeof(unit_t));
		if (in[0].ptr) {
			array_add(a, in[0].ptr);
			free(in[0].ptr);
		}
		out->ptr = a;
	} else {
		if (in[1].ptr) {
			array_add(in[0].ptr, in[1].ptr);
			free(in[1].ptr);
		}
	}
}



REDUCER(notimp) {
	char *buf = strndup(out->first, out->last-out->first);
	fprintf(stderr, "reduction for token %d not implemented <%s>\n", out->id, buf);
	free(buf);
	abort();
}
