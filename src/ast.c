/* Copyright (c) 2015 Fabian Schuiki */
#include "ast.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void
expr_dispose (expr_t *self) {
	if (self == 0)
		return;

	unsigned i;
	switch (self->kind) {
		case AST_IDENT_EXPR:
			free(self->ident);
			break;
		case AST_STRING_LITERAL_EXPR:
			free(self->string_literal);
			break;
		case AST_NUMBER_LITERAL_EXPR:
			free(self->number_literal);
			break;
		case AST_INDEX_ACCESS_EXPR:
			expr_dispose(self->index_access.target);
			expr_dispose(self->index_access.index);
			free(self->index_access.target);
			free(self->index_access.index);
			break;
		case AST_CALL_EXPR:
			expr_dispose(self->index_access.target);
			for (i = 0; i < self->call.num_args; ++i)
				expr_dispose(self->call.args + i);
			free(self->index_access.target);
			free(self->call.args);
			break;
		case AST_MEMBER_ACCESS_EXPR:
			expr_dispose(self->member_access.target);
			free(self->member_access.target);
			free(self->member_access.name);
			break;
		case AST_INCDEC_EXPR:
			expr_dispose(self->incdec_op.target);
			free(self->incdec_op.target);
			break;
		case AST_UNARY_EXPR:
			expr_dispose(self->unary_op.target);
			free(self->unary_op.target);
			break;
		case AST_SIZEOF_EXPR:
			if (self->sizeof_op.mode == AST_EXPR_SIZEOF) {
				expr_dispose(self->sizeof_op.expr);
				free(self->sizeof_op.expr);
			}
			if (self->sizeof_op.mode == AST_TYPE_SIZEOF)
				type_dispose(&self->sizeof_op.type);
			break;
		case AST_NEW_EXPR:
			type_dispose(&self->newe.type);
			break;
		case AST_CAST_EXPR:
			expr_dispose(self->cast.target);
			type_dispose(&self->cast.type);
			free(self->cast.target);
			break;
		case AST_BINARY_EXPR:
			expr_dispose(self->binary_op.lhs);
			expr_dispose(self->binary_op.rhs);
			free(self->binary_op.lhs);
			free(self->binary_op.rhs);
			break;
		case AST_CONDITIONAL_EXPR:
			expr_dispose(self->conditional.condition);
			expr_dispose(self->conditional.true_expr);
			expr_dispose(self->conditional.false_expr);
			free(self->conditional.condition);
			free(self->conditional.true_expr);
			free(self->conditional.false_expr);
			break;
		case AST_ASSIGNMENT_EXPR:
			expr_dispose(self->assignment.target);
			expr_dispose(self->assignment.expr);
			free(self->assignment.target);
			free(self->assignment.expr);
			break;
		case AST_COMMA_EXPR:
			for (i = 0; i < self->comma.num_exprs; ++i)
				expr_dispose(self->comma.exprs + i);
			free(self->comma.exprs);
			break;
		default:
			fprintf(stderr, "%s.%d: expr_dispose for expr kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}


void
stmt_dispose (stmt_t *self) {
	if (self == 0)
		return;

	unsigned i;
	switch (self->kind) {
		case AST_EXPR_STMT:
			expr_dispose(self->expr);
			break;
		case AST_COMPOUND_STMT:
			for (i = 0; i < self->compound.num_items; ++i)
				block_item_dispose(self->compound.items + i);
			free(self->compound.items);
			break;
		case AST_IF_STMT:
		case AST_SWITCH_STMT:
			expr_dispose(self->selection.condition);
			stmt_dispose(self->selection.stmt);
			stmt_dispose(self->selection.else_stmt);
			free(self->selection.condition);
			free(self->selection.stmt);
			free(self->selection.else_stmt);
			break;
		case AST_WHILE_STMT:
		case AST_DO_STMT:
		case AST_FOR_STMT:
			expr_dispose(self->iteration.initial);
			expr_dispose(self->iteration.condition);
			expr_dispose(self->iteration.step);
			stmt_dispose(self->iteration.stmt);
			free(self->iteration.initial);
			free(self->iteration.condition);
			free(self->iteration.step);
			free(self->iteration.stmt);
			break;
		case AST_GOTO_STMT:
			free(self->name);
			break;
		case AST_CONTINUE_STMT:
			break;
		case AST_BREAK_STMT:
			break;
		case AST_RETURN_STMT:
			expr_dispose(self->expr);
			free(self->expr);
			break;
		case AST_LABEL_STMT:
			stmt_dispose(self->label.stmt);
			free(self->label.name);
			free(self->label.stmt);
			break;
		case AST_CASE_STMT:
			expr_dispose(self->label.expr);
			stmt_dispose(self->label.stmt);
			free(self->label.expr);
			free(self->label.stmt);
			break;
		case AST_DEFAULT_STMT:
			stmt_dispose(self->label.stmt);
			free(self->label.stmt);
			break;
		default:
			fprintf(stderr, "%s.%d: stmt_dispose for stmt kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}


void
block_item_dispose (block_item_t *self) {
	if (self == 0)
		return;

	switch (self->kind) {
		case AST_STMT_BLOCK_ITEM:
			stmt_dispose(self->stmt);
			free(self->stmt);
			break;
		case AST_DECL_BLOCK_ITEM:
			decl_dispose(self->decl);
			free(self->decl);
			break;
		default:
			fprintf(stderr, "%s.%d: block_item_dispose for block item kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}


void
decl_dispose (decl_t *self) {
	if (self == 0)
		return;

	switch (self->kind) {
		case AST_VARIABLE_DECL:
			type_dispose(&self->variable.type);
			expr_dispose(self->variable.initial);
			free(self->variable.name);
			free(self->variable.initial);
			break;
		case AST_CONST_DECL:
			type_dispose(self->cons.type);
			expr_dispose(&self->cons.value);
			free(self->cons.name);
			free(self->cons.type);
			break;
		default:
			fprintf(stderr, "%s.%d: decl_dispose for decl kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}


char *
type_describe(type_t *self) {
	assert(self);
	char *s = 0;
	switch (self->kind) {
		case AST_VOID_TYPE:
			s = strdup("void");
			break;
		case AST_BOOLEAN_TYPE:
			s = strdup("bool");
			break;
		case AST_INTEGER_TYPE:
			asprintf(&s, "int%d", self->width);
			break;
		case AST_FLOAT_TYPE:
			asprintf(&s, "float%d", self->width);
			break;
		case AST_FUNC_TYPE: {
			char *ret = type_describe(self->func.return_type);
			char *arg[self->func.num_args];
			unsigned len = strlen(ret);
			len += 2; // " ("
			unsigned i;
			for (i = 0; i < self->func.num_args; ++i) {
				arg[i] = type_describe(self->func.args+i);
				if (i > 0)
					len += 2; // ", "
				len += strlen(arg[i]);
			}
			len += 1; // ")"
			if (self->pointer > 0)
				s += 2; // "()" around func pointers
			s = malloc((len+1) * sizeof(char));
			s[0] = 0;
			if (self->pointer > 0)
				strcat(s, "(");
			strcat(s, ret);
			free(ret);
			strcat(s, " (");
			for (i = 0; i < self->func.num_args; ++i) {
				if (i > 0)
					strcat(s, ", ");
				strcat(s, arg[i]);
				free(arg[i]);
			}
			strcat(s, ")");
			if (self->pointer > 0)
				strcat(s, ")");
			break;
		}
		case AST_NAMED_TYPE:
			s = strdup(self->name);
			break;
		case AST_STRUCT_TYPE:
			s = strdup("struct{...}");
			break;
		case AST_ARRAY_TYPE: {
			char *t = type_describe(self->array.type);
			asprintf(&s, "%s[%d]", t, self->array.length);
			free(t);
			break;
		}
		default:
			fprintf(stderr, "%s.%d: type_describe for type kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
	if (s && self->pointer > 0) {
		char suffix[self->pointer+1];
		memset(suffix, '*', self->pointer);
		suffix[self->pointer] = 0;
		char *sn = 0;
		asprintf(&sn, "%s%s", s, suffix);
		free(s);
		s = sn;
	}
	return s;
}

void
type_copy (type_t *dst, const type_t *src) {
	assert(dst);
	assert(src);
	*dst = *src;
	unsigned i;
	switch (src->kind) {
		case AST_FUNC_TYPE:
			dst->func.return_type = malloc(sizeof(type_t));
			type_copy(dst->func.return_type, src->func.return_type);
			dst->func.args = malloc(src->func.num_args * sizeof(type_t));
			for (i = 0; i < src->func.num_args; ++i)
				type_copy(dst->func.args+i, src->func.args+i);
			break;
		case AST_NAMED_TYPE:
			dst->name = strdup(src->name);
			break;
		case AST_STRUCT_TYPE:
			dst->strct.members = malloc(src->strct.num_members * sizeof(struct_member_t));
			for (i = 0; i < src->strct.num_members; ++i) {
				dst->strct.members[i].type = malloc(sizeof(type_t));
				type_copy(dst->strct.members[i].type, src->strct.members[i].type);
				dst->strct.members[i].name = strdup(src->strct.members[i].name);
			}
		case AST_ARRAY_TYPE:
			type_copy(dst->array.type, src->array.type);
			break;
		default:
			break;
	}
}

int
type_equal (const type_t *a, const type_t *b) {
	if (a->kind != b->kind ||
		a->pointer != b->pointer)
		return 0;

	unsigned i;
	switch (a->kind) {
		case AST_NO_TYPE:
		case AST_VOID_TYPE:
		case AST_BOOLEAN_TYPE:
			return 1;
		case AST_INTEGER_TYPE:
		case AST_FLOAT_TYPE:
			return a->width == b->width;
		case AST_NAMED_TYPE:
			return strcmp(a->name, b->name) == 0;
		case AST_FUNC_TYPE: {
			if (a->func.num_args != b->func.num_args ||
				!type_equal(a->func.return_type, b->func.return_type))
				return 0;
			for (i = 0; i < a->func.num_args; ++i)
				if (!type_equal(a->func.args+i, b->func.args+i))
					return 0;
			return 1;
		}
		case AST_STRUCT_TYPE: {
			if (a->strct.num_members != b->strct.num_members ||
				strcmp(a->strct.name, b->strct.name) != 0)
				return 0;
			for (i = 0; i < a->strct.num_members; ++i)
				if (strcmp(a->strct.members[i].name, b->strct.members[i].name) != 0 ||
					!type_equal(a->strct.members[i].type, b->strct.members[i].type))
					return 0;
			return 1;
		}
		case AST_ARRAY_TYPE:
			return a->array.length == b->array.length || type_equal(a->array.type, b->array.type);
		default:
			fprintf(stderr, "%s.%d: type_equal for type kind %d not implemented\n", __FILE__, __LINE__, a->kind);
			abort();
	}
}

void
type_dispose (type_t *self) {
	if (self == 0)
		return;
	unsigned i;
	switch (self->kind) {
		case AST_VOID_TYPE:
		case AST_BOOLEAN_TYPE:
		case AST_INTEGER_TYPE:
		case AST_FLOAT_TYPE:
			break;
		case AST_FUNC_TYPE:
			type_dispose(self->func.return_type);
			for (i = 0; i < self->func.num_args; ++i)
				type_dispose(self->func.args + i);
			free(self->func.return_type);
			free(self->func.args);
			break;
		case AST_NAMED_TYPE:
			free(self->name);
			break;
		case AST_STRUCT_TYPE:
			for (i = 0; i < self->strct.num_members; ++i) {
				type_dispose(self->strct.members[i].type);
				free(self->strct.members[i].type);
				free(self->strct.members[i].name);
			}
			free(self->strct.members);
			break;
		case AST_ARRAY_TYPE:
			type_dispose(self->array.type);
			free(self->array.type);
			break;
		default:
			fprintf(stderr, "%s.%d: type_dispose for type kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}


void
unit_dispose (unit_t *self) {
	if (self == 0)
		return;
	unsigned i;
	switch (self->kind) {
		case AST_IMPORT_UNIT:
			free(self->import_name);
			break;
		case AST_DECL_UNIT:
			decl_dispose(self->decl);
			free(self->decl);
			break;
		case AST_FUNC_UNIT:
			type_dispose(&self->func.return_type);
			free(self->func.name);
			stmt_dispose(self->func.body);
			free(self->func.body);
			for (i = 0; i < self->func.num_params; ++i) {
				type_dispose(&self->func.params[i].type);
				free(self->func.params[i].name);
			}
			free(self->func.params);
			break;
		case AST_TYPE_UNIT:
			type_dispose(&self->type.type);
			free(self->type.name);
			break;
		default:
			fprintf(stderr, "%s.%d: unit_dispose for unit kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}
