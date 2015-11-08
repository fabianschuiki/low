/* Copyright (c) 2015 Fabian Schuiki */
#include "ast.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void func_def_dispose(func_def_t *self) {
	if (self->name)
		free(self->name);
}

void
expr_dispose (expr_t *self) {
	if (self == 0)
		return;

	unsigned i;
	switch (self->type) {
		case AST_IDENT:
			free(self->ident);
			break;
		case AST_STRING_LITERAL:
			free(self->string_literal);
			break;
		case AST_NUMBER_LITERAL:
			free(self->number_literal);
			break;
		case AST_INDEX_ACCESS:
			expr_dispose(self->index_access.target);
			expr_dispose(self->index_access.index);
			break;
		case AST_CALL:
			expr_dispose(self->index_access.target);
			for (i = 0; i < self->call.num_args; ++i)
				expr_dispose(self->call.args + i);
			free(self->call.args);
			break;
		case AST_MEMBER_ACCESS:
			expr_dispose(self->member_access.target);
			free(self->member_access.name);
			break;
		case AST_INCDEC_OP:
			expr_dispose(self->incdec_op.target);
			break;
		case AST_UNARY_OP:
			expr_dispose(self->unary_op.target);
			break;
		case AST_SIZEOF_OP:
			if (self->sizeof_op.mode == AST_SIZEOF_EXPR)
				expr_dispose(self->sizeof_op.expr);
			if (self->sizeof_op.mode == AST_SIZEOF_TYPE)
				/*type_dispose(self->sizeof_op.type)*/;
			break;
		case AST_CAST:
			expr_dispose(self->cast.target);
			// type_dispose(self->cast.type);
			break;
		case AST_BINARY_OP:
			expr_dispose(self->binary_op.lhs);
			expr_dispose(self->binary_op.rhs);
			break;
		case AST_CONDITIONAL:
			expr_dispose(self->conditional.condition);
			expr_dispose(self->conditional.true_expr);
			expr_dispose(self->conditional.false_expr);
			break;
		case AST_ASSIGNMENT:
			expr_dispose(self->assignment.target);
			expr_dispose(self->assignment.expr);
			break;
		case AST_COMMA_EXPR:
			for (i = 0; i < self->comma_expr.num_exprs; ++i)
				expr_dispose(self->comma_expr.exprs + i);
			free(self->comma_expr.exprs);
			break;
		default:
			fprintf(stderr, "%s.%d: expr_dispose for expr type %d not handled\n", __FILE__, __LINE__, self->type);
			abort();
			break;
	}
}


void
stmt_dispose (stmt_t *self) {
	if (self == 0)
		return;

	unsigned i;
	switch (self->type) {
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
			break;
		case AST_WHILE_STMT:
		case AST_DO_STMT:
		case AST_FOR_STMT:
			expr_dispose(self->iteration.initial);
			expr_dispose(self->iteration.condition);
			expr_dispose(self->iteration.step);
			stmt_dispose(self->iteration.stmt);
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
			break;
		case AST_LABEL_STMT:
			free(self->label.name);
			stmt_dispose(self->label.stmt);
			break;
		case AST_CASE_STMT:
			expr_dispose(self->label.expr);
			stmt_dispose(self->label.stmt);
			break;
		case AST_DEFAULT_STMT:
			stmt_dispose(self->label.stmt);
			break;
		default:
			fprintf(stderr, "%s.%d: stmt_dispose for stmt type %d not handled\n", __FILE__, __LINE__, self->type);
			abort();
			break;
	}
}


void
block_item_dispose (block_item_t *self) {
	if (self == 0)
		return;

	switch (self->type) {
		case AST_STMT_BLOCK_ITEM:
			stmt_dispose(self->stmt);
			break;
		case AST_DECL_BLOCK_ITEM:
			decl_dispose(self->decl);
			break;
		default:
			fprintf(stderr, "%s.%d: block_item_dispose for block item type %d not handled\n", __FILE__, __LINE__, self->type);
			abort();
			break;
	}
}


void
decl_dispose (decl_t *self) {
	if (self == 0)
		return;

	switch (self->type) {
		case AST_VARIABLE_DECL:
			type_dispose(&self->variable.type);
			free(self->variable.name);
			expr_dispose(self->variable.initial);
			free(self->variable.initial);
			break;
		default:
			fprintf(stderr, "%s.%d: decl_dispose for decl type %d not handled\n", __FILE__, __LINE__, self->type);
			abort();
			break;
	}
}


void
type_dispose (type_t *self) {
}


void
unit_dispose (unit_t *self) {
	if (self == 0)
		return;

	switch (self->type) {
		case AST_IMPORT_UNIT:
			free(self->import_name);
			break;
		case AST_DECL_UNIT:
			decl_dispose(self->decl);
			free(self->decl);
			break;
		case AST_FUNC_UNIT:
			free(self->func.name);
			stmt_dispose(self->func.body);
			free(self->func.body);
			break;
		default:
			fprintf(stderr, "%s.%d: unit_dispose for unit type %d not handled\n", __FILE__, __LINE__, self->type);
			abort();
			break;
	}
}
