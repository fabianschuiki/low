/* Copyright (c) 2015-2016 Fabian Schuiki, Thomas Richner */
#include "codegen_internal.h"


/*
 * generates IR to allocate zero initialised array on heap, return pointer to new array
 * similar to `calloc(type,size)`
 */
static LLVMValueRef
codegen_array_new(codegen_t* self, codegen_context_t *context,LLVMTypeRef type,LLVMValueRef size){
	//---- malloc on heap
	LLVMValueRef arrptr = LLVMBuildArrayMalloc(self->builder,type,size,"");

	//TODO check for nonzero

	//---- zero initialise TODO: use memset intrinsic
	LLVMValueRef cntrptr = LLVMBuildAlloca(self->builder,LLVMTypeOf(size),"cntrptr");
	LLVMBuildStore(self->builder,size,cntrptr);

	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(self->builder));
	LLVMBasicBlockRef loop_block = LLVMAppendBasicBlock(func, "loopcond");
	LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(func, "loopbody");
	LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "loopexit");
	LLVMBuildBr(self->builder, loop_block);

	// Check the loop condition.
	LLVMPositionBuilderAtEnd(self->builder, loop_block);
	LLVMValueRef cntr = LLVMBuildLoad(self->builder,cntrptr,"cntr");
	LLVMValueRef cond = LLVMBuildICmp(self->builder,LLVMIntSLE,LLVMConstNull(LLVMTypeOf(cntr)),cntr,"cond");
	LLVMBuildCondBr(self->builder, cond, body_block, exit_block);

	// Execute the loop body.
	LLVMPositionBuilderAtEnd(self->builder, body_block);
	//LLVMValueRef iptr  = LLVMBuildInBoundsGEP(self->builder, arrptr, (LLVMValueRef[]){LLVMConstNull(LLVMInt32Type()), cntrptr}, 2, "");
	LLVMValueRef iptr  = LLVMBuildInBoundsGEP(self->builder, arrptr, (LLVMValueRef[]){cntr}, 1, "");
	LLVMBuildStore(self->builder,LLVMConstNull(type),iptr);
	cntr = LLVMBuildSub(self->builder,cntr,LLVMConstInt(LLVMTypeOf(cntr),1,0),"");
	LLVMBuildStore(self->builder,cntr,cntrptr);
	LLVMBuildBr(self->builder, loop_block);

	LLVMPositionBuilderAtEnd(self->builder, exit_block);

	return arrptr;
}



PREPARE_EXPR(new_builtin_expr) {
	if(expr->newe.expr){
		type_t int_type = {.kind=AST_INTEGER_TYPE,.width=64};
		prepare_expr(self, context, expr->newe.expr, &int_type);
	}
	type_copy(&expr->type, &expr->newe.type);
	++expr->type.pointer;
}

PREPARE_EXPR(free_builtin_expr) {
	prepare_expr(self, context, expr->free.expr, 0);
	expr->type.kind = AST_VOID_TYPE;

	if (expr->free.expr->type.pointer == 0) {
		derror(&expr->loc, "cannot free non-pointer\n");
	}
}

PREPARE_EXPR(make_builtin_expr) {
	type_t *type = resolve_type_name(context, &expr->make.type);
	if (type->kind != AST_SLICE_TYPE) {
		derror(&expr->loc, "cannot make non-slice\n");
	}
	type_t int_type = { .kind = AST_INTEGER_TYPE, .width = 64 };
	prepare_expr(self, context, expr->make.expr, &int_type);
	type_copy(&expr->type, &expr->make.type);
}

PREPARE_EXPR(lencap_builtin_expr) {
	prepare_expr(self,context,expr->lencap.expr, 0);

	type_t *type = resolve_type_name(context, &expr->lencap.expr->type);
	if (type->kind != AST_SLICE_TYPE) {
		derror(&expr->loc, "cannot use with non-slice type\n");
	}

	type_t int_type = {.kind=AST_INTEGER_TYPE,.width=64};
	type_copy(&expr->type, &int_type);
}

PREPARE_EXPR(dispose_builtin_expr) {
	prepare_expr(self, context, expr->dispose.expr, 0);
	expr->type.kind = AST_VOID_TYPE;

	type_t *target_type = resolve_type_name(context, &expr->dispose.expr->type);
	if (target_type->kind != AST_SLICE_TYPE) {
		char *td = type_describe(&expr->dispose.expr->type);
		derror(&expr->loc, "dispose can only be used on slices, got %s\n", td);
		free(td);
	}
}

CODEGEN_EXPR(new_builtin_expr) {
	LLVMTypeRef type = codegen_type(context, &expr->newe.type);

	LLVMValueRef ptr;
	if(expr->newe.expr){
		LLVMValueRef size = codegen_expr(self, context, expr->newe.expr, 0, 0);
		ptr = codegen_array_new(self,context,type,size);
	}else{
		ptr = LLVMBuildMalloc(self->builder,type,"");
		LLVMBuildStore(self->builder,LLVMConstNull(type),ptr);
	}

	return ptr;
}

CODEGEN_EXPR(free_builtin_expr) {
	LLVMValueRef ptr = codegen_expr(self, context, expr->free.expr, 0, 0);
	return LLVMBuildFree(self->builder,ptr);
}

CODEGEN_EXPR(make_builtin_expr) {
	// assert(expr->make.type.kind==AST_SLICE_TYPE && "MAKE only for slices defined");
	type_t *type = resolve_type_name(context, &expr->make.type);
	if (type->kind != AST_SLICE_TYPE) {
		derror(&expr->loc, "cannot make non-slice\n");
	}

	//---- init cap to given value
	LLVMValueRef caparg = codegen_expr(self, context, expr->make.expr, 0, 0);
	caparg = LLVMBuildIntCast(self->builder, caparg, LLVMInt64Type(),"");

	//---- alloc array on heap
	LLVMTypeRef element_type = codegen_type(context, type->slice.type); // array element type
	LLVMValueRef arrptr = codegen_array_new(self,context,element_type,caparg);
	LLVMTypeRef array_type = LLVMPointerType(element_type, 0);
	arrptr = LLVMBuildPointerCast(self->builder,arrptr,array_type,"");

	//---- assemble struct
	LLVMTypeRef make_type = codegen_type(context, type);
	LLVMValueRef slice = LLVMConstNull(make_type);

	slice = LLVMBuildInsertValue(self->builder, slice, arrptr, 0, "arrptr");
	slice = LLVMBuildInsertValue(self->builder, slice, caparg, 2, "cap");
	slice = LLVMBuildInsertValue(self->builder, slice, arrptr, 3, "baseptr");

	return slice;
}

CODEGEN_EXPR(lencap_builtin_expr) {
	LLVMValueRef slice = codegen_expr(self, context, expr->lencap.expr, 0, 0);

	if (expr->lencap.kind == AST_LEN) {
		return LLVMBuildExtractValue(self->builder, slice, 1, "len");
	} else if (expr->lencap.kind == AST_CAP) {
		return LLVMBuildExtractValue(self->builder, slice, 2, "cap");
	} else {
		die("unexpected len/cap kind");
		return 0;
	}
}

CODEGEN_EXPR(dispose_builtin_expr) {
	LLVMValueRef slice_ptr = codegen_expr(self, context, expr->dispose.expr, 1, 0);
	LLVMValueRef slice = LLVMBuildLoad(self->builder, slice_ptr, "");

	LLVMValueRef ptr = LLVMBuildExtractValue(self->builder, slice, 3, "baseptr");

	// set slice to zero
	LLVMBuildStore(self->builder, LLVMConstNull(LLVMTypeOf(slice)), slice_ptr);

	return LLVMBuildFree(self->builder, ptr);
}
