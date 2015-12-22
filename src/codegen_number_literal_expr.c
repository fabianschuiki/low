/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"


DETERMINE_TYPE(number_literal_expr) {
	if (type_hint)
		type_copy(&expr->type, type_hint);
}


CODEGEN_EXPR(number_literal_expr) {
	assert(!lvalue && "number literal is not a valid lvalue");
	if (expr->type.kind == AST_NO_TYPE)
		derror(&expr->loc, "type of literal '%s' cannot be inferred from context, use a cast\n", expr->number_literal);
	if (expr->type.kind == AST_INTEGER_TYPE) {
		return LLVMConstIntOfString(codegen_type(context, &expr->type), expr->number_literal.literal, expr->number_literal.radix);
	} else if (expr->type.kind == AST_FLOAT_TYPE) {
		return LLVMConstRealOfString(codegen_type(context, &expr->type), expr->number_literal.literal);
	} else if (expr->type.kind == AST_BOOLEAN_TYPE) {
		char *p = expr->number_literal.literal;
		while (*p == '0' && *(p+1) != 0) ++p;
		if (strcmp(p, "0") == 0)
			return LLVMConstNull(LLVMInt1Type());
		else if (strcmp(p, "1") == 0)
			return LLVMConstAllOnes(LLVMInt1Type());
		else
			derror(&expr->loc, "'%s' is not a valid boolean number, can only be 0 or 1\n", expr->number_literal.literal);
	} else {
		derror(&expr->loc, "number literal can only be an integer, a boolean, or a float\n");
	}
	return 0;
}
