/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"

#define BOTH(name) \
	DETERMINE_TYPE(name); \
	CODEGEN_EXPR(name);

BOTH(binary_expr);
BOTH(unary_expr);
BOTH(call_expr);
BOTH(member_access_expr);
BOTH(cast_expr);
BOTH(ident_expr);
