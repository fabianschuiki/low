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
BOTH(free_builtin_expr);
BOTH(ident_expr);
BOTH(incdec_expr);
BOTH(index_access_expr);
BOTH(lencap_builtin_expr);
BOTH(make_builtin_expr);
BOTH(member_access_expr);
BOTH(new_builtin_expr);
BOTH(number_literal_expr);
BOTH(sizeof_expr);
BOTH(string_literal_expr);
BOTH(unary_expr);

#undef BOTH


const determine_type_fn_t determine_type_fn[AST_NUM_EXPRS] = {
	[AST_ASSIGNMENT_EXPR]     = determine_type_assignment_expr,
	[AST_BINARY_EXPR]         = determine_type_binary_expr,
	[AST_CALL_EXPR]           = determine_type_call_expr,
	[AST_CAST_EXPR]           = determine_type_cast_expr,
	[AST_CONDITIONAL_EXPR]    = determine_type_conditional_expr,
	[AST_FREE_BUILTIN]        = determine_type_free_builtin_expr,
	[AST_IDENT_EXPR]          = determine_type_ident_expr,
	[AST_INCDEC_EXPR]         = determine_type_incdec_expr,
	[AST_INDEX_ACCESS_EXPR]   = determine_type_index_access_expr,
	[AST_LENCAP_BUILTIN]      = determine_type_lencap_builtin_expr,
	[AST_MAKE_BUILTIN]        = determine_type_make_builtin_expr,
	[AST_MEMBER_ACCESS_EXPR]  = determine_type_member_access_expr,
	[AST_NEW_BUILTIN]         = determine_type_new_builtin_expr,
	[AST_NUMBER_LITERAL_EXPR] = determine_type_number_literal_expr,
	[AST_SIZEOF_EXPR]         = determine_type_sizeof_expr,
	[AST_STRING_LITERAL_EXPR] = determine_type_string_literal_expr,
	[AST_UNARY_EXPR]          = determine_type_unary_expr,
};


const codegen_expr_fn_t codegen_expr_fn[AST_NUM_EXPRS] = {
	[AST_ASSIGNMENT_EXPR]     = codegen_assignment_expr,
	[AST_BINARY_EXPR]         = codegen_binary_expr,
	[AST_CALL_EXPR]           = codegen_call_expr,
	[AST_CAST_EXPR]           = codegen_cast_expr,
	[AST_CONDITIONAL_EXPR]    = codegen_conditional_expr,
	[AST_FREE_BUILTIN]        = codegen_free_builtin_expr,
	[AST_IDENT_EXPR]          = codegen_ident_expr,
	[AST_INCDEC_EXPR]         = codegen_incdec_expr,
	[AST_INDEX_ACCESS_EXPR]   = codegen_index_access_expr,
	[AST_LENCAP_BUILTIN]      = codegen_lencap_builtin_expr,
	[AST_MAKE_BUILTIN]        = codegen_make_builtin_expr,
	[AST_MEMBER_ACCESS_EXPR]  = codegen_member_access_expr,
	[AST_NEW_BUILTIN]         = codegen_new_builtin_expr,
	[AST_NUMBER_LITERAL_EXPR] = codegen_number_literal_expr,
	[AST_SIZEOF_EXPR]         = codegen_sizeof_expr,
	[AST_STRING_LITERAL_EXPR] = codegen_string_literal_expr,
	[AST_UNARY_EXPR]          = codegen_unary_expr,
};
