/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"


DETERMINE_TYPE(incdec_expr) {
	determine_type(self, context, expr->incdec_op.target, type_hint);
	type_copy(&expr->type, &expr->incdec_op.target->type);
}


CODEGEN_EXPR(incdec_expr) {
	assert(expr->incdec_op.order == AST_PRE && "post-increment/-decrement not yet implemented");

	LLVMValueRef target = codegen_expr(self, context, expr->incdec_op.target, 1, 0);
	if (!target) {
		fprintf(stderr, "expr %d is not a valid lvalue and thus cannot be incremented/decremented\n", expr->assignment.target->kind);
		abort();
		return 0;
	}
	// expr->type = expr->incdec_op.target->type;
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
	return lvalue ? target : new_value;
}
