/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"

#define BOTH(name) \
	DETERMINE_TYPE(name); \
	CODEGEN_EXPR(name);

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
