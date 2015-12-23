/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"


PREPARE_TYPE(string_literal_expr) {
	expr->type.kind = AST_INTEGER_TYPE;
	expr->type.width = 8;
	expr->type.pointer = 1;
}


CODEGEN_EXPR(string_literal_expr) {
	assert(!lvalue && "string literal is not a valid lvalue");
	return LLVMBuildGlobalStringPtr(self->builder, expr->string_literal, ".str");
}
