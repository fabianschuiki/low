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


/* struct {
 *    *arr: *[i32 x 0]
 *    len: i64
 *    cap: i64
 *	  off: i64
 * }
 */

static LLVMValueRef
codegen_slice_new(codegen_t *self, codegen_context_t *context, make_builtin_t *make){
	assert(self);
	assert(context);
	assert(make);

	//---- init cap to given value
	LLVMValueRef caparg = codegen_expr(self, context, make->expr, 0, 0);
	caparg = LLVMBuildIntCast(self->builder, caparg, LLVMInt64Type(),"");

	//---- alloc array on heap
	LLVMTypeRef element_type = codegen_type(context, make->type.slice.type); // array element type
	LLVMValueRef arrptr = codegen_array_new(self,context,element_type,caparg);
	LLVMTypeRef array_type = LLVMPointerType(element_type, 0);
	arrptr = LLVMBuildPointerCast(self->builder,arrptr,array_type,"");

	//---- assemble struct
	LLVMTypeRef make_type = codegen_type(context, &make->type);
	LLVMValueRef slice = LLVMConstNull(make_type);

	slice = LLVMBuildInsertValue(self->builder, slice, arrptr, 0, "arrptr");
	slice = LLVMBuildInsertValue(self->builder, slice, caparg, 2, "cap");
	slice = LLVMBuildInsertValue(self->builder, slice, arrptr, 3, "baseptr");

	return slice;
}



PREPARE_EXPR(new_builtin_expr) {
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
	if (expr->make.type.kind != AST_SLICE_TYPE) {
		derror(&expr->loc, "cannot make non-slice\n");
	}
	type_t int_type = {.kind=AST_INTEGER_TYPE,.width=64};
	prepare_expr(self,context,expr->make.expr, &int_type);
	type_copy(&expr->type, &expr->make.type);
}

PREPARE_EXPR(lencap_builtin_expr) {
	prepare_expr(self,context,expr->lencap.expr, 0);

	if (expr->lencap.expr->type.kind != AST_SLICE_TYPE) {
		derror(&expr->loc, "cannot use with non-slice type\n");
	}

	type_t int_type = {.kind=AST_INTEGER_TYPE,.width=64};
	type_copy(&expr->type, &int_type);
}

PREPARE_EXPR(dispose_builtin_expr) {
	prepare_expr(self, context, expr->dispose.expr, 0);
	expr->type.kind = AST_VOID_TYPE;
}

CODEGEN_EXPR(new_builtin_expr) {
	LLVMTypeRef type = codegen_type(context, &expr->newe.type);
	LLVMValueRef ptr = LLVMBuildMalloc(self->builder,type,"");
	LLVMBuildStore(self->builder,LLVMConstNull(type),ptr);
	return ptr;
}

CODEGEN_EXPR(free_builtin_expr) {
	LLVMValueRef ptr = codegen_expr(self, context, expr->free.expr, 0, 0);
	return LLVMBuildFree(self->builder,ptr);
}

CODEGEN_EXPR(make_builtin_expr) {
	assert(expr->make.type.kind==AST_SLICE_TYPE && "MAKE only for slices defined");
	return codegen_slice_new(self,context,&expr->make);
}

CODEGEN_EXPR(lencap_builtin_expr) {
	assert(expr->lencap.expr->type.kind==AST_SLICE_TYPE && "CAP/LEN only for slices defined");
	LLVMValueRef aslice = codegen_expr(self, context, expr->lencap.expr, 0, 0);

	LLVMValueRef eptr = NULL;
	if(expr->lencap.kind==AST_LEN){
		eptr = LLVMBuildStructGEP(self->builder,aslice,1,"len");
	}else if(expr->lencap.kind==AST_CAP){
		eptr = LLVMBuildStructGEP(self->builder,aslice,2,"cap");
	}else{
		derror(&expr->loc,"what a terrible failure :(");
	}

	return LLVMBuildLoad(self->builder,eptr,"");
}

CODEGEN_EXPR(dispose_builtin_expr) {

	assert(expr->dispose.expr->type.kind==AST_SLICE_TYPE && "DISPOSE only for slices defined");
	LLVMValueRef aslice = codegen_expr(self, context, expr->dispose.expr, 0, 0);

	LLVMValueRef eptr	= LLVMBuildStructGEP(self->builder,aslice,3,"baseptr");
	LLVMValueRef arrptr = LLVMBuildLoad(self->builder,eptr,"arrptr");

	// set slice to zero
	LLVMValueRef slice = LLVMBuildLoad(self->builder,aslice,""); // can we do this without load?
	LLVMBuildStore(self->builder,LLVMConstNull(LLVMTypeOf(slice)),aslice);

	return LLVMBuildFree(self->builder,arrptr);
}
