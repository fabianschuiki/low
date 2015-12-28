/* Copyright (c) 2015 Fabian Schuiki, Thomas Richner */
#include "codegen_internal.h"

#define BOTH(name) \
	PREPARE_TYPE(name); \
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
BOTH(dispose_builtin_expr);
BOTH(make_builtin_expr);
BOTH(member_access_expr);
BOTH(new_builtin_expr);
BOTH(number_literal_expr);
BOTH(sizeof_expr);
BOTH(string_literal_expr);
BOTH(unary_expr);

#undef BOTH


const prepare_expr_fn_t prepare_expr_fn[AST_NUM_EXPRS] = {
	[AST_ASSIGNMENT_EXPR]     = prepare_assignment_expr,
	[AST_BINARY_EXPR]         = prepare_binary_expr,
	[AST_CALL_EXPR]           = prepare_call_expr,
	[AST_CAST_EXPR]           = prepare_cast_expr,
	[AST_CONDITIONAL_EXPR]    = prepare_conditional_expr,
	[AST_FREE_BUILTIN]        = prepare_free_builtin_expr,
	[AST_IDENT_EXPR]          = prepare_ident_expr,
	[AST_INCDEC_EXPR]         = prepare_incdec_expr,
	[AST_INDEX_ACCESS_EXPR]   = prepare_index_access_expr,
	[AST_LENCAP_BUILTIN]      = prepare_lencap_builtin_expr,
	[AST_DISPOSE_BUILTIN]     = prepare_dispose_builtin_expr,
	[AST_MAKE_BUILTIN]        = prepare_make_builtin_expr,
	[AST_MEMBER_ACCESS_EXPR]  = prepare_member_access_expr,
	[AST_NEW_BUILTIN]         = prepare_new_builtin_expr,
	[AST_NUMBER_LITERAL_EXPR] = prepare_number_literal_expr,
	[AST_SIZEOF_EXPR]         = prepare_sizeof_expr,
	[AST_STRING_LITERAL_EXPR] = prepare_string_literal_expr,
	[AST_UNARY_EXPR]          = prepare_unary_expr,
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
	[AST_DISPOSE_BUILTIN]     = codegen_dispose_builtin_expr,
	[AST_MAKE_BUILTIN]        = codegen_make_builtin_expr,
	[AST_MEMBER_ACCESS_EXPR]  = codegen_member_access_expr,
	[AST_NEW_BUILTIN]         = codegen_new_builtin_expr,
	[AST_NUMBER_LITERAL_EXPR] = codegen_number_literal_expr,
	[AST_SIZEOF_EXPR]         = codegen_sizeof_expr,
	[AST_STRING_LITERAL_EXPR] = codegen_string_literal_expr,
	[AST_UNARY_EXPR]          = codegen_unary_expr,
};


LLVMTypeRef
codegen_type(codegen_context_t *context, type_t *type);