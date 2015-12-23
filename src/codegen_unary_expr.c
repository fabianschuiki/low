/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"


PREPARE_TYPE(unary_expr) {
	switch (expr->unary_op.op) {
		case AST_ADDRESS: {
			type_t *inner_hint = 0;
			type_t hint;
			if (type_hint && type_hint->pointer > 0) {
				type_copy(&hint, type_hint);
				--hint.pointer;
				inner_hint = &hint;
			}
			prepare_expr(self, context, expr->unary_op.target, inner_hint);
			type_copy(&expr->type, &expr->unary_op.target->type);
			++expr->type.pointer;
		} break;

		case AST_DEREF: {
			if (type_hint) {
				type_t new_hint;
				type_copy(&new_hint, type_hint);
				++new_hint.pointer;
				prepare_expr(self, context, expr->unary_op.target, &new_hint);
			} else {
				prepare_expr(self, context, expr->unary_op.target, 0);
			}
			type_copy(&expr->type, &expr->unary_op.target->type);
			expr->type = expr->unary_op.target->type;
			if (expr->type.pointer == 0)
				derror(&expr->loc, "cannot dereference non-pointer\n");
			--expr->type.pointer;
		} break;

		case AST_POSITIVE:
		case AST_NEGATIVE:
		case AST_BITWISE_NOT: {
			prepare_expr(self, context, expr->unary_op.target, type_hint);
			type_copy(&expr->type, &expr->unary_op.target->type);
		} break;

		case AST_NOT: {
			type_t bool_type = { .kind = AST_BOOLEAN_TYPE };
			prepare_expr(self, context, expr->unary_op.target, &bool_type);
			bzero(&expr->type, sizeof expr->type);
			expr->type.kind = AST_BOOLEAN_TYPE;
		} break;

		default:
			die("type determination for unary op %d not implemented\n", expr->unary_op.op);
	}
}


CODEGEN_EXPR(unary_expr) {
	type_t *type = &expr->unary_op.target->type;
	switch (expr->unary_op.op) {
		LLVMValueRef target;

		case AST_ADDRESS:
			return codegen_expr(self, context, expr->unary_op.target, 1, 0);

		case AST_DEREF:
			target = codegen_expr(self, context, expr->unary_op.target, 0, 0);
			return lvalue ? target : LLVMBuildLoad(self->builder, target, "");

		case AST_POSITIVE:
			return codegen_expr(self, context, expr->unary_op.target, 0, 0);

		case AST_NEGATIVE:
			target = codegen_expr(self, context, expr->unary_op.target, 0, 0);
			if (type->kind == AST_INTEGER_TYPE) {
				return LLVMBuildNeg(self->builder, target, "");
			} else if (type->kind == AST_FLOAT_TYPE) {
				return LLVMBuildFNeg(self->builder, target, "");
			} else {
				char *td = type_describe(type);
				derror(&expr->loc, "expression of type %s cannot be negated\n", td);
				free(td);
			}
			break;

		case AST_BITWISE_NOT:
			if (type->kind == AST_INTEGER_TYPE) {
				target = codegen_expr(self, context, expr->unary_op.target, 0, 0);
				return LLVMBuildNot(self->builder, target, "");
			} else {
				char *td = type_describe(type);
				derror(&expr->loc, "unary operator not available for type %s\n", td);
				free(td);
			}
			break;

		case AST_NOT:
			if (type->kind == AST_BOOLEAN_TYPE) {
				target = codegen_expr(self, context, expr->unary_op.target, 0, 0);
				return LLVMBuildNot(self->builder, target, "");
			} else {
				char *td = type_describe(type);
				derror(&expr->loc, "unary operator not available for type %s\n", td);
				free(td);
			}
			break;

		default:
			die("codegen for unary op %d not implemented\n", expr->unary_op.op);
	}
	return 0;
}
