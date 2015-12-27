/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"


PREPARE_TYPE(incdec_expr) {
	prepare_expr(self, context, expr->incdec_op.target, type_hint);
	type_copy(&expr->type, &expr->incdec_op.target->type);
}


CODEGEN_EXPR(incdec_expr) {
	if (lvalue)
		derror(&expr->loc, "result of increment/decrement operation is not a valid lvalue\n");

	LLVMValueRef target = codegen_expr(self, context, expr->incdec_op.target, 1, 0);
	if (!target)
		derror(&expr->loc, "expression is not a valid lvalue and thus cannot be incremented/decremented\n");
	if (expr->type.kind != AST_INTEGER_TYPE)
		derror(&expr->loc, "increment/decrement operator only supported for integers\n");

	LLVMValueRef value = LLVMBuildLoad(self->builder, target, "");
	LLVMValueRef const_one = LLVMConstInt(codegen_type(context, &expr->type), 1, 0);
	LLVMValueRef new_value;
	if (expr->incdec_op.direction == AST_INC) {
		new_value = LLVMBuildAdd(self->builder, value, const_one, "");
	} else {
		new_value = LLVMBuildSub(self->builder, value, const_one, "");
	}

	LLVMBuildStore(self->builder, new_value, target);
	return expr->incdec_op.order == AST_PRE ? new_value : value;
}
