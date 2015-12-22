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
	[AST_ASSIGNMENT_EXPR]     = determine_type_assignment_expr,
	[AST_BINARY_EXPR]         = determine_type_binary_expr,
	[AST_CALL_EXPR]           = determine_type_call_expr,
	[AST_CAST_EXPR]           = determine_type_cast_expr,
	[AST_CONDITIONAL_EXPR]    = determine_type_conditional_expr,
	[AST_IDENT_EXPR]          = determine_type_ident_expr,
	[AST_INDEX_ACCESS_EXPR]   = determine_type_index_access_expr,
	[AST_MEMBER_ACCESS_EXPR]  = determine_type_member_access_expr,
	[AST_NUMBER_LITERAL_EXPR] = determine_type_number_literal_expr,
	[AST_STRING_LITERAL_EXPR] = determine_type_string_literal_expr,
	[AST_UNARY_EXPR]          = determine_type_unary_expr,
};


const codegen_expr_fn_t codegen_expr_fn[AST_NUM_EXPRS] = {
	[AST_ASSIGNMENT_EXPR]     = codegen_assignment_expr,
	[AST_BINARY_EXPR]         = codegen_binary_expr,
	[AST_CALL_EXPR]           = codegen_call_expr,
	[AST_CAST_EXPR]           = codegen_cast_expr,
	[AST_CONDITIONAL_EXPR]    = codegen_conditional_expr,
	[AST_IDENT_EXPR]          = codegen_ident_expr,
	[AST_INDEX_ACCESS_EXPR]   = codegen_index_access_expr,
	[AST_MEMBER_ACCESS_EXPR]  = codegen_member_access_expr,
	[AST_NUMBER_LITERAL_EXPR] = codegen_number_literal_expr,
	[AST_STRING_LITERAL_EXPR] = codegen_string_literal_expr,
	[AST_UNARY_EXPR]          = codegen_unary_expr,
};
