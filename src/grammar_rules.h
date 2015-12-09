/* Copyright (c) 2015 Fabian Schuiki */
#define GRAMMAR \
\
/* --- expr ---------------------------------------------------------------- */\
\
RULE(primary_expr) \
	VAR TKN(IDENT) REDUCE(primary_expr_ident) \
	VAR TKN(STRING_LITERAL) REDUCE(primary_expr_string) \
	VAR TKN(NUMBER_LITERAL) REDUCE(primary_expr_number) \
	VAR TKN(LPAREN) SUB(expr) TKN(RPAREN) REDUCE(primary_expr_paren) \
RULE_END \
\
RULE(postfix_expr) \
	VAR SUB(primary_expr) REDUCE_DEFAULT \
	VAR SUB(postfix_expr) TKN(LBRACK) SUB(expr) TKN(RBRACK) REDUCE(postfix_expr_index) \
	VAR SUB(postfix_expr) TKN(LPAREN) TKN(RPAREN) REDUCE_TAG(postfix_expr_call, 0) \
	VAR SUB(postfix_expr) TKN(LPAREN) SUB(argument_expr_list) TKN(RPAREN) REDUCE_TAG(postfix_expr_call, 1) \
	VAR SUB(postfix_expr) TKN(PERIOD) TKN(IDENT) REDUCE(postfix_expr_member) \
	VAR SUB(postfix_expr) TKN(INC_OP) REDUCE_TAG(postfix_expr_incdec, 0) \
	VAR SUB(postfix_expr) TKN(DEC_OP) REDUCE_TAG(postfix_expr_incdec, 1) \
	/*VAR TKN(LPAREN) SUB(type) TKN(RPAREN) TKN(LBRACE) SUB(initializer_list) TKN(RBRACE) REDUCE(postfix_expr_initializer)*/ \
	/*VAR TKN(LPAREN) SUB(type) TKN(RPAREN) TKN(LBRACE) SUB(initializer_list) TKN(COMMA) TKN(RBRACE) REDUCE(postfix_expr_initializer)*/ \
RULE_END \
\
RULE(argument_expr_list) \
	VAR SUB(assignment_expr) REDUCE_TAG(argument_expr_list, 0) \
	VAR SUB(argument_expr_list) TKN(COMMA) SUB(assignment_expr) REDUCE_TAG(argument_expr_list, 1) \
RULE_END \
\
RULE(unary_expr) \
	VAR SUB(postfix_expr) REDUCE_DEFAULT \
	VAR TKN(INC_OP) SUB(unary_expr) REDUCE_TAG(unary_expr_incdec, 0) \
	VAR TKN(DEC_OP) SUB(unary_expr) REDUCE_TAG(unary_expr_incdec, 1) \
	VAR SUB(unary_op) SUB(cast_expr) REDUCE(unary_expr_op) \
	VAR TKN(SIZEOF) SUB(unary_expr) REDUCE_TAG(unary_expr_sizeof, 0) \
	/*TODO: add parenthesis */ \
	VAR TKN(SIZEOF) TKN(HASH) SUB(type) REDUCE_TAG(unary_expr_sizeof, 1) \
	VAR SUB(builtin_func) REDUCE_DEFAULT \
RULE_END \
\
RULE(builtin_func) \
	VAR TKN(NEW) TKN(LPAREN) SUB(type) TKN(RPAREN) REDUCE(builtin_func_new) \
	VAR TKN(FREE) TKN(LPAREN) SUB(expr) TKN(RPAREN) REDUCE(builtin_func_free) \
	VAR TKN(MAKE) TKN(LPAREN) SUB(type) TKN(COMMA) SUB(expr) TKN(RPAREN) REDUCE(builtin_func_make) \
RULE_END \
\
RULE(unary_op) \
	VAR TKN(BITWISE_AND) REDUCE(unary_op) \
	VAR TKN(MUL_OP) REDUCE(unary_op) \
	VAR TKN(ADD_OP) REDUCE(unary_op) \
	VAR TKN(SUB_OP) REDUCE(unary_op) \
	VAR TKN(BITWISE_NOT) REDUCE(unary_op) \
	VAR TKN(NOT_OP) REDUCE(unary_op) \
RULE_END \
\
RULE(cast_expr) \
	VAR SUB(unary_expr) REDUCE_DEFAULT \
	VAR TKN(HASH) TKN(LPAREN) SUB(type) TKN(RPAREN) SUB(cast_expr) REDUCE(cast_expr) \
	VAR TKN(HASH) SUB(type) TKN(LPAREN) SUB(expr) TKN(RPAREN) REDUCE(cast_expr2) \
RULE_END \
\
RULE(multiplicative_expr) \
	VAR SUB(cast_expr) REDUCE_DEFAULT \
	VAR SUB(multiplicative_expr) TKN(MUL_OP) SUB(cast_expr) REDUCE_TAG(binary_expr, 0) \
	VAR SUB(multiplicative_expr) TKN(DIV_OP) SUB(cast_expr) REDUCE_TAG(binary_expr, 0) \
	VAR SUB(multiplicative_expr) TKN(MOD_OP) SUB(cast_expr) REDUCE_TAG(binary_expr, 0) \
RULE_END \
\
RULE(additive_expr) \
	VAR SUB(multiplicative_expr) REDUCE_DEFAULT \
	VAR SUB(additive_expr) TKN(ADD_OP) SUB(multiplicative_expr) REDUCE_TAG(binary_expr, 1) \
	VAR SUB(additive_expr) TKN(SUB_OP) SUB(multiplicative_expr) REDUCE_TAG(binary_expr, 1) \
RULE_END \
\
RULE(shift_expr) \
	VAR SUB(additive_expr) REDUCE_DEFAULT \
	VAR SUB(shift_expr) TKN(LEFT_OP) SUB(additive_expr) REDUCE_TAG(binary_expr, 2) \
	VAR SUB(shift_expr) TKN(RIGHT_OP) SUB(additive_expr) REDUCE_TAG(binary_expr, 2) \
RULE_END \
\
RULE(relational_expr) \
	VAR SUB(shift_expr) REDUCE_DEFAULT \
	VAR SUB(relational_expr) TKN(LT_OP) SUB(shift_expr) REDUCE_TAG(binary_expr, 3) \
	VAR SUB(relational_expr) TKN(GT_OP) SUB(shift_expr) REDUCE_TAG(binary_expr, 3) \
	VAR SUB(relational_expr) TKN(LE_OP) SUB(shift_expr) REDUCE_TAG(binary_expr, 3) \
	VAR SUB(relational_expr) TKN(GE_OP) SUB(shift_expr) REDUCE_TAG(binary_expr, 3) \
RULE_END \
\
RULE(equality_expr) \
	VAR SUB(relational_expr) REDUCE_DEFAULT \
	VAR SUB(equality_expr) TKN(EQ_OP) SUB(relational_expr) REDUCE_TAG(binary_expr, 4) \
	VAR SUB(equality_expr) TKN(NE_OP) SUB(relational_expr) REDUCE_TAG(binary_expr, 4) \
RULE_END \
\
RULE(bitwise_and_expr) \
	VAR SUB(equality_expr) REDUCE_DEFAULT \
	VAR SUB(bitwise_and_expr) TKN(BITWISE_AND) SUB(equality_expr) REDUCE_TAG(binary_expr, 5) \
RULE_END \
\
RULE(bitwise_xor_expr) \
	VAR SUB(bitwise_and_expr) REDUCE_DEFAULT \
	VAR SUB(bitwise_xor_expr) TKN(BITWISE_XOR) SUB(bitwise_and_expr) REDUCE_TAG(binary_expr, 6) \
RULE_END \
\
RULE(bitwise_or_expr) \
	VAR SUB(bitwise_xor_expr) REDUCE_DEFAULT \
	VAR SUB(bitwise_or_expr) TKN(BITWISE_OR) SUB(bitwise_xor_expr) REDUCE_TAG(binary_expr, 7) \
RULE_END \
\
RULE(and_expr) \
	VAR SUB(bitwise_or_expr) REDUCE_DEFAULT \
	VAR SUB(and_expr) TKN(AND_OP) SUB(bitwise_or_expr) REDUCE_TAG(binary_expr, 8) \
RULE_END \
\
RULE(or_expr) \
	VAR SUB(and_expr) REDUCE_DEFAULT \
	VAR SUB(or_expr) TKN(OR_OP) SUB(and_expr) REDUCE_TAG(binary_expr, 9) \
RULE_END \
\
RULE(conditional_expr) \
	VAR SUB(or_expr) REDUCE_DEFAULT \
	VAR SUB(or_expr) TKN(QUESTION) SUB(expr) TKN(COLON) SUB(conditional_expr) REDUCE(conditional_expr) \
RULE_END \
\
RULE(assignment_expr) \
	VAR SUB(conditional_expr) REDUCE_DEFAULT \
	VAR SUB(unary_expr) SUB(assignment_op) SUB(assignment_expr) REDUCE(assignment_expr) \
RULE_END \
\
RULE(assignment_op) \
	VAR TKN(ASSIGN) REDUCE(assignment_op) \
	VAR TKN(ADD_ASSIGN) REDUCE(assignment_op) \
	VAR TKN(AND_ASSIGN) REDUCE(assignment_op) \
	VAR TKN(DIV_ASSIGN) REDUCE(assignment_op) \
	VAR TKN(LEFT_ASSIGN) REDUCE(assignment_op) \
	VAR TKN(MOD_ASSIGN) REDUCE(assignment_op) \
	VAR TKN(MUL_ASSIGN) REDUCE(assignment_op) \
	VAR TKN(OR_ASSIGN) REDUCE(assignment_op) \
	VAR TKN(RIGHT_ASSIGN) REDUCE(assignment_op) \
	VAR TKN(SUB_ASSIGN) REDUCE(assignment_op) \
	VAR TKN(XOR_ASSIGN) REDUCE(assignment_op) \
RULE_END \
\
RULE(expr) \
	VAR SUB(assignment_expr) REDUCE_DEFAULT \
	VAR SUB(expr) TKN(COMMA) SUB(assignment_expr) REDUCE(expr_comma) \
RULE_END \
\
\
\
/* --- stmt ---------------------------------------------------------------- */\
\
RULE(expr_stmt) \
	VAR TKN(SEMICOLON) REDUCE_DEFAULT \
	VAR SUB(expr) TKN(SEMICOLON) REDUCE(expr_stmt) \
RULE_END \
\
RULE(compound_stmt) \
	VAR TKN(LBRACE) TKN(RBRACE) REDUCE_DEFAULT \
	VAR TKN(LBRACE) SUB(block_item_list) TKN(RBRACE) REDUCE(compound_stmt) \
RULE_END \
\
RULE(block_item_list) \
	VAR SUB(block_item) REDUCE_TAG(block_item_list, 0) \
	VAR SUB(block_item_list) SUB(block_item) REDUCE_TAG(block_item_list, 1) \
RULE_END \
\
RULE(block_item) \
	VAR SUB(decl) REDUCE(block_item_decl) \
	VAR SUB(stmt) REDUCE(block_item_stmt) \
RULE_END \
\
RULE(selection_stmt) \
	VAR TKN(IF) TKN(LPAREN) SUB(expr) TKN(RPAREN) SUB(stmt) REDUCE_TAG(selection_stmt_if, 0) \
	VAR TKN(IF) TKN(LPAREN) SUB(expr) TKN(RPAREN) SUB(stmt) TKN(ELSE) SUB(stmt) REDUCE_TAG(selection_stmt_if, 1) \
	VAR TKN(SWITCH) TKN(LPAREN) SUB(expr) TKN(RPAREN) SUB(stmt) REDUCE(selection_stmt_switch) \
RULE_END \
\
RULE(iteration_stmt) \
	VAR TKN(WHILE) TKN(LPAREN) SUB(expr) TKN(RPAREN) SUB(stmt) REDUCE(iteration_stmt_while) \
	VAR TKN(DO) SUB(stmt) TKN(WHILE) TKN(LPAREN) SUB(expr) TKN(RPAREN) TKN(SEMICOLON) REDUCE(iteration_stmt_do) \
	VAR TKN(FOR) TKN(LPAREN) SUB(expr_stmt) SUB(expr_stmt) SUB(expr) TKN(RPAREN) SUB(stmt) REDUCE_TAG(iteration_stmt_for, 0) \
	VAR TKN(FOR) TKN(LPAREN) SUB(expr_stmt) SUB(expr_stmt) TKN(RPAREN) SUB(stmt) REDUCE_TAG(iteration_stmt_for, 1) \
RULE_END \
\
RULE(jump_stmt) \
	VAR TKN(GOTO) TKN(IDENT) TKN(SEMICOLON) REDUCE(jump_stmt_goto) \
	VAR TKN(CONTINUE) TKN(SEMICOLON) REDUCE(jump_stmt_continue) \
	VAR TKN(BREAK) TKN(SEMICOLON) REDUCE(jump_stmt_break) \
	VAR TKN(RETURN) TKN(SEMICOLON) REDUCE_TAG(jump_stmt_return, 0) \
	VAR TKN(RETURN) SUB(expr) TKN(SEMICOLON) REDUCE_TAG(jump_stmt_return, 1) \
RULE_END \
\
RULE(labeled_stmt) \
	/*VAR TKN(IDENT) TKN(COLON) SUB(stmt) REDUCE(labeled_stmt_name)*/ \
	VAR TKN(CASE) SUB(expr) TKN(COLON) SUB(stmt) REDUCE(labeled_stmt_case) \
	VAR TKN(DEFAULT) TKN(COLON) SUB(stmt) REDUCE(labeled_stmt_default) \
RULE_END \
\
RULE(stmt) \
	VAR SUB(expr_stmt) REDUCE_DEFAULT \
	VAR SUB(compound_stmt) REDUCE_DEFAULT \
	VAR SUB(selection_stmt) REDUCE_DEFAULT \
	VAR SUB(iteration_stmt) REDUCE_DEFAULT \
	VAR SUB(jump_stmt) REDUCE_DEFAULT \
	VAR SUB(labeled_stmt) REDUCE_DEFAULT \
RULE_END \
\
\
\
/* --- decl ---------------------------------------------------------------- */\
\
RULE(variable_decl) \
	VAR TKN(VAR) SUB(type) TKN(IDENT) TKN(SEMICOLON) REDUCE_TAG(variable_decl, 0) \
	VAR TKN(VAR) SUB(type) TKN(IDENT) TKN(ASSIGN) SUB(assignment_expr) TKN(SEMICOLON) REDUCE_TAG(variable_decl, 1) \
	VAR TKN(VAR) TKN(IDENT) TKN(ASSIGN) SUB(assignment_expr) TKN(SEMICOLON) REDUCE_TAG(variable_decl, 2) \
	VAR TKN(IDENT) TKN(COLON) SUB(type) TKN(SEMICOLON) REDUCE_TAG(variable_decl2, 0) \
	VAR TKN(IDENT) TKN(COLON) SUB(type) TKN(ASSIGN) SUB(assignment_expr) TKN(SEMICOLON) REDUCE_TAG(variable_decl2, 1) \
	VAR TKN(IDENT) TKN(DEF_ASSIGN) SUB(assignment_expr) TKN(SEMICOLON) REDUCE_TAG(variable_decl2, 2) \
RULE_END \
\
RULE(const_decl) \
	VAR TKN(CONST) TKN(IDENT) TKN(ASSIGN) SUB(assignment_expr) TKN(SEMICOLON) REDUCE_TAG(const_decl, 0) \
	VAR TKN(CONST) TKN(IDENT) SUB(type) TKN(ASSIGN) SUB(assignment_expr) TKN(SEMICOLON) REDUCE_TAG(const_decl, 1) \
RULE_END \
\
RULE(decl) \
	VAR SUB(variable_decl) REDUCE_DEFAULT \
	VAR SUB(const_decl) REDUCE_DEFAULT \
RULE_END \
\
\
\
/* --- type ---------------------------------------------------------------- */\
\
RULE(type) \
	VAR TKN(VOID) REDUCE(type_void) \
	VAR TKN(IDENT) REDUCE(type_name) \
	VAR SUB(type) TKN(MUL_OP) REDUCE(type_pointer) \
	VAR TKN(STRUCT) TKN(LBRACE) SUB(struct_member_list) TKN(RBRACE) REDUCE(type_struct) \
	VAR TKN(LBRACK) TKN(NUMBER_LITERAL) TKN(RBRACK) SUB(type) REDUCE(type_array) \
	VAR TKN(LBRACK) TKN(RBRACK) SUB(type) REDUCE(type_slice) \
	VAR TKN(FUNC) TKN(LPAREN) TKN(RPAREN) SUB(type) REDUCE_TAG(type_func,0) \
	VAR TKN(FUNC) TKN(LPAREN) SUB(func_type_args) TKN(RPAREN) SUB(type) REDUCE_TAG(type_func,1) \
RULE_END \
\
RULE(struct_member_list) \
	VAR SUB(struct_member) REDUCE_TAG(struct_member_list, 0) \
	VAR SUB(struct_member_list) SUB(struct_member) REDUCE_TAG(struct_member_list, 1) \
RULE_END \
\
RULE(struct_member) \
	VAR SUB(type) TKN(IDENT) TKN(SEMICOLON) REDUCE(struct_member) \
RULE_END \
\
RULE(func_type_args) \
	VAR SUB(type) REDUCE_TAG(func_type_args, 0) \
	VAR SUB(func_type_args) TKN(COMMA) SUB(type) REDUCE_TAG(func_type_args, 1) \
RULE_END \
\
\
\
/* --- unit ---------------------------------------------------------------- */\
\
RULE(import_unit) \
	VAR TKN(IMPORT) TKN(STRING_LITERAL) TKN(SEMICOLON) REDUCE(import_unit) \
RULE_END \
\
RULE(decl_unit) \
	VAR SUB(decl) REDUCE(decl_unit) \
RULE_END \
\
RULE(func_unit) \
	VAR SUB(func_unit_decl) TKN(SEMICOLON) REDUCE_TAG(func_unit, 0) \
	VAR SUB(func_unit_decl) SUB(compound_stmt) REDUCE_TAG(func_unit, 1) \
RULE_END \
\
RULE(func_unit_decl) \
	VAR SUB(type) TKN(IDENT) TKN(LPAREN) TKN(RPAREN) REDUCE_TAG(func_unit_decl, 0) \
	VAR SUB(type) TKN(IDENT) TKN(LPAREN) TKN(ELLIPSIS) TKN(RPAREN) REDUCE_TAG(func_unit_decl, 1) \
	VAR SUB(type) TKN(IDENT) TKN(LPAREN) SUB(parameter_list) TKN(RPAREN) REDUCE_TAG(func_unit_decl, 2) \
	VAR SUB(type) TKN(IDENT) TKN(LPAREN) SUB(parameter_list) TKN(COMMA) TKN(ELLIPSIS) TKN(RPAREN) REDUCE_TAG(func_unit_decl, 3) \
	VAR TKN(FUNC) TKN(IDENT) TKN(LPAREN) TKN(RPAREN) SUB(type) REDUCE_TAG(func_unit_decl2, 0) \
	VAR TKN(FUNC) TKN(IDENT) TKN(LPAREN) TKN(ELLIPSIS) TKN(RPAREN) SUB(type) REDUCE_TAG(func_unit_decl2, 1) \
	VAR TKN(FUNC) TKN(IDENT) TKN(LPAREN) SUB(parameter_list) TKN(RPAREN) SUB(type) REDUCE_TAG(func_unit_decl2, 2) \
	VAR TKN(FUNC) TKN(IDENT) TKN(LPAREN) SUB(parameter_list) TKN(COMMA) TKN(ELLIPSIS) TKN(RPAREN) SUB(type) REDUCE_TAG(func_unit_decl2, 3) \
RULE_END \
\
RULE(parameter_list) \
	VAR SUB(parameter) REDUCE_TAG(parameter_list, 0) \
	VAR SUB(parameter_list) TKN(COMMA) SUB(parameter) REDUCE_TAG(parameter_list, 1) \
RULE_END \
\
RULE(parameter) \
	VAR SUB(type) REDUCE_TAG(parameter, 0) \
	VAR SUB(type) TKN(IDENT) REDUCE_TAG(parameter, 1) \
RULE_END \
\
RULE(type_unit) \
	VAR TKN(TYPE) TKN(IDENT) TKN(COLON) SUB(type) TKN(SEMICOLON) REDUCE(type_unit) \
RULE_END \
\
RULE(unit_list) \
	VAR SUB(unit) REDUCE_TAG(unit_list, 0) \
	VAR SUB(unit_list) SUB(unit) REDUCE_TAG(unit_list, 1) \
RULE_END \
\
RULE(unit) \
	VAR SUB(import_unit) REDUCE_DEFAULT \
	VAR SUB(decl_unit) REDUCE_DEFAULT \
	VAR SUB(func_unit) REDUCE_DEFAULT \
	VAR SUB(type_unit) REDUCE_DEFAULT \
	VAR TKN(SEMICOLON) REDUCE_DEFAULT \
RULE_END \
\
\
\
/* --- other stuff --------------------------------------------------------- */\
\
RULE(decl_specifiers) \
	VAR SUB(storage_class_specifier) SUB(decl_specifiers) REDUCE_DEFAULT \
	VAR SUB(type) SUB(decl_specifiers) REDUCE_DEFAULT \
	VAR SUB(type_qualifier) SUB(decl_specifiers) REDUCE_DEFAULT \
	VAR SUB(func_specifier) SUB(decl_specifiers) REDUCE_DEFAULT \
	VAR SUB(alignment_specifier) SUB(decl_specifiers) REDUCE_DEFAULT \
	VAR SUB(storage_class_specifier) REDUCE_DEFAULT \
	VAR SUB(type) REDUCE_DEFAULT \
	VAR SUB(type_qualifier) REDUCE_DEFAULT \
	VAR SUB(func_specifier) REDUCE_DEFAULT \
	VAR SUB(alignment_specifier) REDUCE_DEFAULT \
RULE_END \
\
RULE(init_declarator_list) \
	VAR SUB(init_declarator) REDUCE_DEFAULT \
	VAR SUB(init_declarator_list) TKN(COMMA) SUB(init_declarator) REDUCE_DEFAULT \
RULE_END \
\
RULE(init_declarator) \
	VAR SUB(declarator) TKN(ASSIGN) SUB(initializer) REDUCE_DEFAULT \
	VAR SUB(declarator) REDUCE_DEFAULT \
RULE_END \
\
RULE(declarator) \
	VAR TKN(IDENT) REDUCE_DEFAULT \
RULE_END \
\
RULE(storage_class_specifier) \
	VAR TKN(EXTERN) REDUCE_DEFAULT \
	VAR TKN(STATIC) REDUCE_DEFAULT \
RULE_END \
\
RULE(func_specifier) \
	VAR TKN(INLINE) REDUCE_DEFAULT \
RULE_END \
\
RULE(type_qualifier) \
	VAR TKN(CONST) REDUCE_DEFAULT \
	VAR TKN(VOLATILE) REDUCE_DEFAULT \
	VAR TKN(ATOMIC) REDUCE_DEFAULT \
RULE_END \
\
RULE(alignment_specifier) \
	VAR TKN(ALIGNAS) TKN(LPAREN) SUB(type) TKN(RPAREN) REDUCE_DEFAULT \
	VAR TKN(ALIGNAS) TKN(LPAREN) SUB(expr) TKN(RPAREN) REDUCE_DEFAULT \
RULE_END \
\
RULE(initializer) \
	VAR TKN(LBRACE) SUB(initializer_list) TKN(RBRACE) REDUCE_DEFAULT \
	VAR TKN(LBRACE) SUB(initializer_list) TKN(COMMA) TKN(RBRACE) REDUCE_DEFAULT \
	VAR SUB(assignment_expr) REDUCE_DEFAULT \
RULE_END \
\
RULE(initializer_list) \
	VAR SUB(initializer) REDUCE_DEFAULT \
	VAR SUB(initializer_list) TKN(COMMA) SUB(initializer) REDUCE_DEFAULT \
RULE_END \

// The root node of the grammar.
#define GRAMMAR_ROOT unit_list
