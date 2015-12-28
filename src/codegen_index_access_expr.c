/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"
#include "llvm_intrinsics.h"


static void
build_assert(codegen_t *self,codegen_context_t *context, LLVMValueRef cond){
	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(self->builder));
	LLVMBasicBlockRef true_block = LLVMAppendBasicBlock(func, "iftrue");
	LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "ifexit");

	cond = LLVMBuildNot(self->builder,cond,"");
	LLVMBuildCondBr(self->builder, cond, true_block, exit_block);

	LLVMPositionBuilderAtEnd(self->builder, true_block);

	LLVMValueRef fn = LLVMGetIntrinsicByID(self->module,LLVMIntrinsicIDTrap,0,0);

	assert(fn && "Intrinsic function not found!");

	LLVMBuildCall(self->builder,fn,0,0,"");

	LLVMBuildBr(self->builder, exit_block);

	LLVMPositionBuilderAtEnd(self->builder, exit_block);
}


PREPARE_TYPE(index_access_expr) {
	type_t int_type = { .kind = AST_INTEGER_TYPE, .pointer = 0, .width = 64 };
	prepare_expr(self, context, expr->index_access.target, 0);
	prepare_expr(self, context, expr->index_access.index, &int_type);
	type_t *target = &expr->index_access.target->type;
	if (target->pointer > 0) {
		type_copy(&expr->type, target);
		--expr->type.pointer;
	} else if (target->kind == AST_ARRAY_TYPE) {
		type_copy(&expr->type, target->array.type);
	} else if (target->kind == AST_SLICE_TYPE) {
		type_copy(&expr->type, target->slice.type);
	} else {
		derror(&expr->loc, "cannot index into non-pointer\n");
	}
}


CODEGEN_EXPR(index_access_expr) {
	LLVMValueRef target = codegen_expr(self, context, expr->index_access.target, 0, 0);
	LLVMValueRef index = codegen_expr(self, context, expr->index_access.index, 0, 0);
	if (expr->index_access.index->type.kind != AST_INTEGER_TYPE){
		derror(&expr->index_access.index->loc, "index needs to be an integer\n");
	}

	LLVMValueRef ptr;
	if (expr->index_access.target->type.pointer > 0) {
		ptr = LLVMBuildInBoundsGEP(self->builder, target, &index, 1, "");
	} else if (expr->index_access.target->type.kind == AST_ARRAY_TYPE) {
		ptr = LLVMBuildInBoundsGEP(self->builder, target, (LLVMValueRef[]){LLVMConstNull(LLVMInt32Type()), index}, 2, "");
	} else if (expr->index_access.target->type.kind == AST_SLICE_TYPE) {

		LLVMValueRef capptr = LLVMBuildStructGEP(self->builder, target, 2, "");
		LLVMValueRef cap = LLVMBuildLoad(self->builder,capptr,"");

		LLVMTypeRef dst = LLVMTypeOf(cap);

		index = LLVMBuildIntCast(self->builder, index, dst,"");

		// check idx vs cap
		LLVMValueRef oob = LLVMBuildICmp(self->builder, LLVMIntULT, index, cap, "");
		build_assert(self,context,oob);

		LLVMValueRef arrptrptr = LLVMBuildStructGEP(self->builder, target, 0, "");
		LLVMValueRef arrptr = LLVMBuildLoad(self->builder,arrptrptr,"");

		ptr = LLVMBuildInBoundsGEP(self->builder, arrptr, (LLVMValueRef[]){LLVMConstNull(LLVMInt32Type()), index}, 2, "");

	} else {
		derror(&expr->loc, "cannot index into non-pointer or non-array");
	}
	return lvalue ? ptr : LLVMBuildLoad(self->builder, ptr, "");
}
