/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"

#define BOTH(name) \
	DETERMINE_TYPE(name); \
	CODEGEN_EXPR(name);

BOTH(assignment_expr);
BOTH(binary_expr);
BOTH(call_expr);
BOTH(cast_expr);
BOTH(conditional_expr);
BOTH(ident_expr);
BOTH(index_access_expr);
BOTH(member_access_expr);
BOTH(number_literal_expr);
BOTH(string_literal_expr);
BOTH(unary_expr);

#undef BOTH

const determine_type_fn_t determine_type_fn[AST_NUM_EXPRS] = {
	[AST_ASSIGNMENT_EXPR] = determine_type_assignment_expr,
};

const codegen_expr_fn_t codegen_expr_fn[AST_NUM_EXPRS] = {
	[AST_ASSIGNMENT_EXPR] = codegen_assignment_expr,
};
