/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"


DETERMINE_TYPE(binary_expr) {
	type_t *hint = 0;
	if (expr->binary_op.op == AST_ADD ||
		expr->binary_op.op == AST_SUB ||
		expr->binary_op.op == AST_MUL ||
		expr->binary_op.op == AST_DIV)
		hint = type_hint;

	determine_type(self, context, expr->binary_op.lhs, hint);
	determine_type(self, context, expr->binary_op.rhs, hint);
	if (expr->binary_op.lhs->type.kind == AST_NO_TYPE &&
		expr->binary_op.rhs->type.kind != AST_NO_TYPE)
		determine_type(self, context, expr->binary_op.lhs, &expr->binary_op.rhs->type);
	if (expr->binary_op.rhs->type.kind == AST_NO_TYPE &&
		expr->binary_op.lhs->type.kind != AST_NO_TYPE)
		determine_type(self, context, expr->binary_op.rhs, &expr->binary_op.lhs->type);

	if (expr->binary_op.lhs->type.kind == AST_NO_TYPE &&
		expr->binary_op.rhs->type.kind == AST_NO_TYPE)
		derror(&expr->loc, "cannot determine type of binary operation, use a cast\n");

	switch (expr->binary_op.op) {
		case AST_LT:
		case AST_GT:
		case AST_LE:
		case AST_GE:
		case AST_EQ:
		case AST_NE:
		case AST_AND:
		case AST_OR:
			bzero(&expr->type, sizeof expr->type);
			expr->type.kind = AST_BOOLEAN_TYPE;
			break;
		default:
			type_copy(&expr->type, &expr->binary_op.lhs->type);
			break;
	}
}


CODEGEN_EXPR(binary_expr) {
	assert(!lvalue && "result of a binary operation is not a valid lvalue");
	LLVMValueRef lhs = codegen_expr(self, context, expr->binary_op.lhs, 0, 0);
	LLVMValueRef rhs = codegen_expr(self, context, expr->binary_op.rhs, 0, 0);
	if (!type_equal(&expr->binary_op.lhs->type, &expr->binary_op.rhs->type))
		derror(&expr->loc, "operands to binary operator must be of the same type");

	type_t *t = &expr->binary_op.lhs->type;
	if (t->kind == AST_BOOLEAN_TYPE) {
		switch (expr->binary_op.op) {
			case AST_EQ:
				return LLVMBuildICmp(self->builder, LLVMIntEQ, lhs, rhs, "");
			case AST_NE:
				return LLVMBuildICmp(self->builder, LLVMIntNE, lhs, rhs, "");
			case AST_AND:
				return LLVMBuildAnd(self->builder, lhs, rhs, "");
			case AST_OR:
				return LLVMBuildOr(self->builder, lhs, rhs, "");
			default:
				fprintf(stderr, "%s.%d: codegen for binary op %d on boolean types not implemented\n", __FILE__, __LINE__, expr->binary_op.op);
				abort();
		}
	} else if (t->kind == AST_INTEGER_TYPE) {
		switch (expr->binary_op.op) {
			case AST_ADD:
				return LLVMBuildAdd(self->builder, lhs, rhs, "");
			case AST_SUB:
				return LLVMBuildSub(self->builder, lhs, rhs, "");
			case AST_MUL:
				return LLVMBuildMul(self->builder, lhs, rhs, "");
			case AST_DIV:
				return LLVMBuildSDiv(self->builder, lhs, rhs, "");
			case AST_MOD:
				return LLVMBuildSRem(self->builder, lhs, rhs, "");
			case AST_LEFT:
				return LLVMBuildShl(self->builder, lhs, rhs, "");
			case AST_RIGHT:
				return LLVMBuildAShr(self->builder, lhs, rhs, "");
			case AST_LT:
				return LLVMBuildICmp(self->builder, LLVMIntSLT, lhs, rhs, "");
			case AST_GT:
				return LLVMBuildICmp(self->builder, LLVMIntSGT, lhs, rhs, "");
			case AST_LE:
				return LLVMBuildICmp(self->builder, LLVMIntSLE, lhs, rhs, "");
			case AST_GE:
				return LLVMBuildICmp(self->builder, LLVMIntSGE, lhs, rhs, "");
			case AST_EQ:
				return LLVMBuildICmp(self->builder, LLVMIntEQ, lhs, rhs, "");
			case AST_NE:
				return LLVMBuildICmp(self->builder, LLVMIntNE, lhs, rhs, "");
			case AST_BITWISE_AND:
				return LLVMBuildAnd(self->builder, lhs, rhs, "");
			case AST_BITWISE_XOR:
				return LLVMBuildXor(self->builder, lhs, rhs, "");
			case AST_BITWISE_OR:
				return LLVMBuildOr(self->builder, lhs, rhs, "");
			case AST_AND:
			case AST_OR:
				derror(&expr->loc, "binary operator not available for integer types\n");
			default:
				fprintf(stderr, "%s.%d: codegen for binary op %d on integer types not implemented\n", __FILE__, __LINE__, expr->binary_op.op);
				abort();
		}
	} else if (t->kind == AST_FLOAT_TYPE) {
		switch (expr->binary_op.op) {
			case AST_ADD:
				return LLVMBuildFAdd(self->builder, lhs, rhs, "");
			case AST_SUB:
				return LLVMBuildFSub(self->builder, lhs, rhs, "");
			case AST_MUL:
				return LLVMBuildFMul(self->builder, lhs, rhs, "");
			case AST_DIV:
				return LLVMBuildFDiv(self->builder, lhs, rhs, "");
			case AST_MOD:
				return LLVMBuildFRem(self->builder, lhs, rhs, "");
			case AST_LT:
				return LLVMBuildFCmp(self->builder, LLVMRealOLT, lhs, rhs, "");
			case AST_GT:
				return LLVMBuildFCmp(self->builder, LLVMRealOGT, lhs, rhs, "");
			case AST_LE:
				return LLVMBuildFCmp(self->builder, LLVMRealOLE, lhs, rhs, "");
			case AST_GE:
				return LLVMBuildFCmp(self->builder, LLVMRealOGE, lhs, rhs, "");
			case AST_EQ:
				return LLVMBuildFCmp(self->builder, LLVMRealOEQ, lhs, rhs, "");
			case AST_NE:
				return LLVMBuildFCmp(self->builder, LLVMRealONE, lhs, rhs, "");
			case AST_LEFT:
			case AST_RIGHT:
			case AST_BITWISE_AND:
			case AST_BITWISE_XOR:
			case AST_BITWISE_OR:
			case AST_AND:
			case AST_OR:
				derror(&expr->loc, "binary operator not available for floating point types\n");
			default:
				fprintf(stderr, "%s.%d: codegen for binary op %d on floating point types not implemented\n", __FILE__, __LINE__, expr->binary_op.op);
				abort();
		}
	} else {
		char *td = type_describe(t);
		derror(&expr->loc, "invalid type %s to binary operator\n", td);
		free(td);
		return 0;
	}
}
