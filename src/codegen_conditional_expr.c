/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"


DETERMINE_TYPE(conditional_expr) {
	type_t bool_type = { .kind = AST_BOOLEAN_TYPE };
	determine_type(self, context, expr->conditional.condition, &bool_type);

	determine_type(self, context, expr->conditional.true_expr, type_hint);
	determine_type(self, context, expr->conditional.false_expr, type_hint);

	if (!type_equal(&expr->conditional.true_expr->type, &expr->conditional.false_expr->type))
		derror(&expr->loc, "true and false expression of conditional must be of the same type");

	type_copy(&expr->type, &expr->conditional.true_expr->type);
}


CODEGEN_EXPR(conditional_expr) {
	assert(!lvalue && "result of a conditional is not a valid lvalue");
	LLVMValueRef cond = codegen_expr(self, context, expr->conditional.condition, 0, 0);

	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(self->builder));
	LLVMBasicBlockRef true_block = LLVMAppendBasicBlock(func, "iftrue");
	LLVMBasicBlockRef false_block = LLVMAppendBasicBlock(func, "iffalse");
	LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "ifexit");
	LLVMBuildCondBr(self->builder, cond, true_block, false_block ? false_block : exit_block);

	LLVMPositionBuilderAtEnd(self->builder, true_block);
	LLVMValueRef true_value = codegen_expr(self, context, expr->conditional.true_expr, 0, 0);
	LLVMBuildBr(self->builder, exit_block);

	LLVMPositionBuilderAtEnd(self->builder, false_block);
	LLVMValueRef false_value = codegen_expr(self, context, expr->conditional.false_expr, 0, 0);
	LLVMBuildBr(self->builder, exit_block);

	// TODO: Make sure types of true and false branch match, otherwise cast.
	expr->type = expr->conditional.true_expr->type;

	LLVMPositionBuilderAtEnd(self->builder, exit_block);
	LLVMValueRef incoming_values[2] = { true_value, false_value };
	LLVMBasicBlockRef incoming_blocks[2] = { true_block, false_block };
	LLVMValueRef phi = LLVMBuildPhi(self->builder, codegen_type(context, &expr->conditional.true_expr->type), "");
	LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);
	return phi;
}
