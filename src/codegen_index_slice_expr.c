/* Copyright (c) 2015-2016 Fabian Schuiki, Thomas Richner */
#include "codegen_internal.h"


PREPARE_EXPR(index_slice_expr) {
	type_t int_type = { .kind = AST_INTEGER_TYPE, .pointer = 0, .width = 64 };

	prepare_expr(self, context, expr->index_slice.target, 0);
	if (expr->index_slice.start)
		prepare_expr(self, context, expr->index_slice.start, &int_type);
	if (expr->index_slice.end)
		prepare_expr(self, context, expr->index_slice.end, &int_type);

	type_t *target = &expr->index_slice.target->type;
	if (target->kind == AST_SLICE_TYPE) {
		type_copy(&expr->type, target);
	} else if (target->kind == AST_ARRAY_TYPE) {
		bzero(&expr->type, sizeof(type_t));
		expr->type.kind = AST_SLICE_TYPE;
		expr->type.slice.type = malloc(sizeof(type_t));
		type_copy(expr->type.slice.type, target->array.type);
	} else {
		char *td = type_describe(&expr->index_slice.target->type);
		derror(&expr->loc, "expression of type %s cannot be sliced\n", td);
		free(td);
	}
}


/* Slices an existing slice.
 *
 * New values:
 * ncap = end - start
 * nlen = max(len - start,0)
 * narrptr = arrptr + start
 * nbaseptr = baseptr
 */
static LLVMValueRef slice_slice(codegen_t *self, codegen_context_t *context, expr_t *expr) {
	LLVMValueRef target = codegen_expr(self, context, expr->index_slice.target, 0, 0);
	LLVMValueRef zero = LLVMConstNull(LLVMInt64Type());

	//---- simple copy?
	if(expr->index_slice.kind==AST_INDEX_SLICE_COPY){
		LLVMValueRef slice = LLVMBuildLoad(self->builder,target,"slice");
		return slice;
	}

	//---- codegen start and end expr
	LLVMValueRef start 	= 0;
	LLVMValueRef end 	= 0;
	if(expr->index_slice.start){
		if (expr->index_slice.start->type.kind != AST_INTEGER_TYPE){
			derror(&expr->index_slice.start->loc, "index needs to be an integer\n");
		}
		start = codegen_expr(self, context, expr->index_slice.start, 0, 0);

	}else{
		start = zero;
	}
	if(expr->index_slice.end){
		if (expr->index_slice.end->type.kind != AST_INTEGER_TYPE){
			derror(&expr->index_slice.end->loc, "index needs to be an integer\n");
		}
		end = codegen_expr(self, context, expr->index_slice.end, 0, 0);
	}else{
		end = zero;
	}

	//---- load previous cap and len
	LLVMValueRef capptr = LLVMBuildStructGEP(self->builder, target, 2, "");
	LLVMValueRef cap = LLVMBuildLoad(self->builder,capptr,"");

	LLVMValueRef lenptr = LLVMBuildStructGEP(self->builder, target, 1, "");
	LLVMValueRef len = LLVMBuildLoad(self->builder,lenptr,"");

	LLVMTypeRef tcap = LLVMTypeOf(cap);

	start = LLVMBuildIntCast(self->builder, start, tcap,"");
	end = LLVMBuildIntCast(self->builder, end, tcap,"");

	//---- calc new len/cap
	LLVMValueRef ncap = LLVMBuildSub(self->builder, end, start, "");
	LLVMValueRef nlen = LLVMBuildSub(self->builder, len, start, "nlen");

	//---- calc new array ptr
	LLVMValueRef arrptrptr = LLVMBuildStructGEP(self->builder, target, 0, "");
	LLVMValueRef arrptr = LLVMBuildLoad(self->builder,arrptrptr,"");
	LLVMValueRef ptr = LLVMBuildInBoundsGEP(self->builder, arrptr, &start, 1, "");

	// cast to pointer type
	LLVMTypeRef array_type = LLVMTypeOf(arrptr);
	ptr = LLVMBuildPointerCast(self->builder,ptr,array_type,"");

	LLVMValueRef baseptrptr = LLVMBuildStructGEP(self->builder, target, 3, "");
	LLVMValueRef baseptr = LLVMBuildLoad(self->builder,baseptrptr,"");

	//---- assemble new struct
	LLVMValueRef oldslice = LLVMBuildLoad(self->builder,target,"old_slice");
	LLVMTypeRef tslice = LLVMTypeOf(oldslice);
	LLVMValueRef slice = LLVMConstNull(tslice);

	slice = LLVMBuildInsertValue(self->builder, slice, ptr, 0, "slice");
	slice = LLVMBuildInsertValue(self->builder, slice, ncap, 2, "slice");
	slice = LLVMBuildInsertValue(self->builder, slice, baseptr, 3, "slice");

	//---- len = max(0, len)
	LLVMValueRef cond = LLVMBuildICmp(self->builder, LLVMIntSGT,nlen,zero,"min");

	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(self->builder));
	LLVMBasicBlockRef true_block  = LLVMAppendBasicBlock(func, "iftrue");
	LLVMBasicBlockRef false_block = LLVMAppendBasicBlock(func, "iffalse");
	LLVMBasicBlockRef exit_block  = LLVMAppendBasicBlock(func, "ifexit");

	LLVMBuildCondBr(self->builder, cond, true_block, false_block);
	LLVMPositionBuilderAtEnd(self->builder, true_block);

	LLVMValueRef slice_len = LLVMBuildInsertValue(self->builder, slice, nlen, 1, "slice_len");

	LLVMBuildBr(self->builder, exit_block);

	// needed for PHI
	LLVMPositionBuilderAtEnd(self->builder, false_block);
	LLVMBuildBr(self->builder, exit_block);

	LLVMPositionBuilderAtEnd(self->builder, exit_block);

	LLVMValueRef incoming_values[2] = { slice_len, slice };
	LLVMBasicBlockRef incoming_blocks[2] = { true_block, false_block };
	LLVMValueRef phi = LLVMBuildPhi(self->builder, tslice, "");
	LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);

	return phi;
}


static LLVMValueRef slice_array(codegen_t *self, codegen_context_t *context, expr_t *expr) {
	expr_t *target = expr->index_slice.target;

	// Determine the type of the "ptr" field in the slice struct.
	LLVMTypeRef array_ptr_type = LLVMPointerType(codegen_type(context, expr->type.slice.type), 0);

	// Get a pointer to the array that is of the right type and determine the
	// length of the array as a value that can be passed to LLVM instructions.
	LLVMValueRef array_ptr = LLVMBuildPointerCast(self->builder, codegen_expr(self, context, target, 1, 0), array_ptr_type, "");
	LLVMValueRef array_length = LLVMConstInt(LLVMInt64Type(), target->type.array.length, 0);

	// Determine the start index of the slice: Either 0 or the index provided.
	LLVMValueRef index_start;
	if (expr->index_slice.start) {
		index_start = codegen_expr(self, context, expr->index_slice.start, 0, 0);
	} else {
		index_start = LLVMConstNull(LLVMInt64Type());
	}

	// Determine the end index of the slice: Either the array length or the
	// index provided.
	LLVMValueRef index_end;
	if (expr->index_slice.end) {
		index_end = codegen_expr(self, context, expr->index_slice.end, 0, 0);
	} else {
		index_end = array_length;
	}

	// Calculate the pointer to the first element of the slice, which can be
	// obtained by shifting the array pointer index_start elements.
	LLVMValueRef ptr = LLVMBuildInBoundsGEP(
		self->builder, array_ptr,
		&index_start, 1,
		""
	);

	// Calculate the length and capacity of the slice, which are (end-start) and
	// (array_length-start) respectively.
	LLVMValueRef len = LLVMBuildSub(self->builder, index_end, index_start, "");
	LLVMValueRef cap = LLVMBuildSub(self->builder, array_length, index_start, "");

	// Assemble the slice struct.
	LLVMValueRef result = LLVMConstNull(codegen_type(context, &expr->type));
	result = LLVMBuildInsertValue(self->builder, result, ptr, 0, ""); // ptr
	result = LLVMBuildInsertValue(self->builder, result, len, 1, ""); // len
	result = LLVMBuildInsertValue(self->builder, result, cap, 2, ""); // cap
	result = LLVMBuildInsertValue(self->builder, result, array_ptr, 3, ""); // base
	return result;
}


CODEGEN_EXPR(index_slice_expr) {
	unsigned kind = expr->index_slice.target->type.kind;
	if (kind == AST_SLICE_TYPE) {
		return slice_slice(self, context, expr);
	} else if (kind == AST_ARRAY_TYPE) {
		return slice_array(self, context, expr);
	} else {
		die("invalid slicing target\n");
		return 0;
	}
}
