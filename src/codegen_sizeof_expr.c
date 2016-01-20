/* Copyright (c) 2015-2016 Fabian Schuiki */
#include "codegen_internal.h"


PREPARE_EXPR(sizeof_expr) {
	if (expr->sizeof_op.mode == AST_EXPR_SIZEOF)
		prepare_expr(self, context, expr->sizeof_op.expr, 0);
	if (type_hint)
		type_copy(&expr->type, type_hint);
}


CODEGEN_EXPR(sizeof_expr) {
	assert(!lvalue && "result of sizeof expression is not a valid lvalue");
	if (expr->type.kind == AST_NO_TYPE)
		derror(&expr->loc, "return type of sizeof() cannot be inferred from context, use a cast\n");
	LLVMValueRef v;
	switch (expr->sizeof_op.mode) {
		case AST_EXPR_SIZEOF:
			v = LLVMSizeOf(LLVMTypeOf(codegen_expr(self, context, expr->sizeof_op.expr, 0, 0)));
			break;
		case AST_TYPE_SIZEOF:
			v = LLVMSizeOf(codegen_type(context, &expr->sizeof_op.type));
			break;
		default:
			fprintf(stderr, "%s.%d: codegen for sizeof mode %d not implemented\n", __FILE__, __LINE__, expr->sizeof_op.mode);
			abort();
	}
	return LLVMBuildIntCast(self->builder, v, codegen_type(context, &expr->type), "");
}
