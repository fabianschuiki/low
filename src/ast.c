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
			for (i = 0; i < self->comma_expr.num_exprs; ++i)
				expr_dispose(self->comma_expr.exprs + i);
			free(self->comma_expr.exprs);
			break;
		default:
			fprintf(stderr, "%s.%d: expr_dispose for expr type %d not handled\n", __FILE__, __LINE__, self->kind);
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
			fprintf(stderr, "%s.%d: stmt_dispose for stmt type %d not handled\n", __FILE__, __LINE__, self->kind);
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
			fprintf(stderr, "%s.%d: block_item_dispose for block item type %d not handled\n", __FILE__, __LINE__, self->kind);
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
		default:
			fprintf(stderr, "%s.%d: decl_dispose for decl type %d not handled\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}


char *
type_describe(type_t *self) {
	char *s = 0;
	switch (self->kind) {
		case AST_VOID_TYPE:
			s = strdup("void");
			break;
		case AST_INTEGER_TYPE:
			asprintf(&s, "int%d", self->width);
			break;
		case AST_FLOAT_TYPE:
			asprintf(&s, "float%d", self->width);
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
	*dst = *src;
}

void
type_dispose (type_t *self) {
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
		default:
			fprintf(stderr, "%s.%d: unit_dispose for unit type %d not handled\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}
