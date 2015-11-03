/* Copyright (c) 2015 Fabian Schuiki */
#define GRAMMAR \
\
RULE(primary_expr) \
	VAR TKN(IDENT) VAR_END() \
	VAR TKN(STRING_LITERAL) VAR_END() \
	VAR TKN(NUMBER_LITERAL) VAR_END() \
	VAR TKN(LPAREN) SUB(expr) TKN(RPAREN) VAR_END() \
RULE_END \
\
RULE(postfix_expr) \
	VAR SUB(primary_expr) VAR_END() \
	VAR SUB(postfix_expr) TKN(LBRACK) SUB(expr) TKN(RBRACK) VAR_END() \
	VAR SUB(postfix_expr) TKN(LPAREN) TKN(RPAREN) VAR_END() \
	VAR SUB(postfix_expr) TKN(LPAREN) SUB(argument_expr_list) TKN(RPAREN) VAR_END() \
	VAR SUB(postfix_expr) TKN(PERIOD) TKN(IDENT) VAR_END() \
	VAR SUB(postfix_expr) TKN(INC_OP) VAR_END() \
	VAR SUB(postfix_expr) TKN(DEC_OP) VAR_END() \
	VAR TKN(LPAREN) SUB(type) TKN(RPAREN) TKN(LBRACE) SUB(initializer_list) TKN(RBRACE) VAR_END() \
	VAR TKN(LPAREN) SUB(type) TKN(RPAREN) TKN(LBRACE) SUB(initializer_list) TKN(COMMA) TKN(RBRACE) VAR_END() \
RULE_END \
\
RULE(argument_expr_list) \
	VAR SUB(assignment_expr) VAR_END() \
	VAR SUB(argument_expr_list) TKN(COMMA) SUB(assignment_expr) VAR_END() \
RULE_END \
\
RULE(unary_expr) \
	VAR SUB(postfix_expr) VAR_END() \
	VAR TKN(INC_OP) SUB(unary_expr) VAR_END() \
	VAR TKN(DEC_OP) SUB(unary_expr) VAR_END() \
	VAR SUB(unary_op) SUB(cast_expr) VAR_END() \
	VAR TKN(SIZEOF) SUB(unary_expr) VAR_END() \
	VAR TKN(SIZEOF) TKN(LPAREN) SUB(type) TKN(RPAREN) VAR_END() \
RULE_END \
\
RULE(unary_op) \
	VAR TKN(BITWISE_AND) VAR_END() \
	VAR TKN(MUL_OP) VAR_END() \
	VAR TKN(ADD_OP) VAR_END() \
	VAR TKN(SUB_OP) VAR_END() \
	VAR TKN(BITWISE_NOT) VAR_END() \
	VAR TKN(NOT_OP) VAR_END() \
RULE_END \
\
RULE(cast_expr) \
	VAR SUB(unary_expr) VAR_END() \
	VAR TKN(LPAREN) SUB(type) TKN(RPAREN) SUB(cast_expr) VAR_END() \
RULE_END \
\
RULE(multiplicative_expr) \
	VAR SUB(cast_expr) VAR_END() \
	VAR SUB(multiplicative_expr) TKN(MUL_OP) SUB(cast_expr) VAR_END() \
	VAR SUB(multiplicative_expr) TKN(DIV_OP) SUB(cast_expr) VAR_END() \
	VAR SUB(multiplicative_expr) TKN(MOD_OP) SUB(cast_expr) VAR_END() \
RULE_END \
\
RULE(additive_expr) \
	VAR SUB(multiplicative_expr) VAR_END() \
	VAR SUB(additive_expr) TKN(ADD_OP) SUB(multiplicative_expr) VAR_END() \
	VAR SUB(additive_expr) TKN(SUB_OP) SUB(multiplicative_expr) VAR_END() \
RULE_END \
\
RULE(shift_expr) \
	VAR SUB(additive_expr) VAR_END() \
	VAR SUB(shift_expr) TKN(LEFT_OP) SUB(additive_expr) VAR_END() \
	VAR SUB(shift_expr) TKN(RIGHT_OP) SUB(additive_expr) VAR_END() \
RULE_END \
\
RULE(relational_expr) \
	VAR SUB(shift_expr) VAR_END() \
	VAR SUB(relational_expr) TKN(LT_OP) SUB(shift_expr) VAR_END() \
	VAR SUB(relational_expr) TKN(GT_OP) SUB(shift_expr) VAR_END() \
	VAR SUB(relational_expr) TKN(LE_OP) SUB(shift_expr) VAR_END() \
	VAR SUB(relational_expr) TKN(GE_OP) SUB(shift_expr) VAR_END() \
RULE_END \
\
RULE(equality_expr) \
	VAR SUB(relational_expr) VAR_END() \
	VAR SUB(equality_expr) TKN(EQ_OP) SUB(relational_expr) VAR_END() \
	VAR SUB(equality_expr) TKN(NE_OP) SUB(relational_expr) VAR_END() \
RULE_END \
\
RULE(bitwise_and_expr) \
	VAR SUB(equality_expr) VAR_END() \
	VAR SUB(bitwise_and_expr) TKN(BITWISE_AND) SUB(equality_expr) VAR_END() \
RULE_END \
\
RULE(bitwise_xor_expr) \
	VAR SUB(bitwise_and_expr) VAR_END() \
	VAR SUB(bitwise_xor_expr) TKN(BITWISE_XOR) SUB(bitwise_and_expr) VAR_END() \
RULE_END \
\
RULE(bitwise_or_expr) \
	VAR SUB(bitwise_xor_expr) VAR_END() \
	VAR SUB(bitwise_or_expr) TKN(BITWISE_OR) SUB(bitwise_xor_expr) VAR_END() \
RULE_END \
\
RULE(and_expr) \
	VAR SUB(bitwise_or_expr) VAR_END() \
	VAR SUB(and_expr) TKN(AND_OP) SUB(bitwise_or_expr) VAR_END() \
RULE_END \
\
RULE(or_expr) \
	VAR SUB(and_expr) VAR_END() \
	VAR SUB(or_expr) TKN(OR_OP) SUB(and_expr) VAR_END() \
RULE_END \
\
RULE(conditional_expr) \
	VAR SUB(or_expr) VAR_END() \
	VAR SUB(or_expr) TKN(QUESTION) SUB(expr) TKN(COLON) SUB(conditional_expr) VAR_END() \
RULE_END \
\
RULE(assignment_expr) \
	VAR SUB(conditional_expr) VAR_END() \
	VAR SUB(unary_expr) SUB(assignment_op) SUB(assignment_expr) VAR_END() \
RULE_END \
\
RULE(assignment_op) \
	VAR TKN(ASSIGN) VAR_END() \
	VAR TKN(ADD_ASSIGN) VAR_END() \
	VAR TKN(AND_ASSIGN) VAR_END() \
	VAR TKN(DIV_ASSIGN) VAR_END() \
	VAR TKN(LEFT_ASSIGN) VAR_END() \
	VAR TKN(MOD_ASSIGN) VAR_END() \
	VAR TKN(MUL_ASSIGN) VAR_END() \
	VAR TKN(OR_ASSIGN) VAR_END() \
	VAR TKN(RIGHT_ASSIGN) VAR_END() \
	VAR TKN(SUB_ASSIGN) VAR_END() \
	VAR TKN(XOR_ASSIGN) VAR_END() \
RULE_END \
\
RULE(expr) \
	VAR SUB(assignment_expr) VAR_END() \
	VAR SUB(expr) TKN(COMMA) SUB(assignment_expr) VAR_END() \
RULE_END \
\
RULE(decl) \
	VAR SUB(decl_specifiers) TKN(SEMICOLON) VAR_END() \
	VAR SUB(decl_specifiers) SUB(init_declarator_list) TKN(SEMICOLON) VAR_END() \
RULE_END \
\
RULE(decl_specifiers) \
	VAR SUB(storage_class_specifier) SUB(decl_specifiers) VAR_END() \
	VAR SUB(type) SUB(decl_specifiers) VAR_END() \
	VAR SUB(type_qualifier) SUB(decl_specifiers) VAR_END() \
	VAR SUB(func_specifier) SUB(decl_specifiers) VAR_END() \
	VAR SUB(alignment_specifier) SUB(decl_specifiers) VAR_END() \
	VAR SUB(storage_class_specifier) VAR_END() \
	VAR SUB(type) VAR_END() \
	VAR SUB(type_qualifier) VAR_END() \
	VAR SUB(func_specifier) VAR_END() \
	VAR SUB(alignment_specifier) VAR_END() \
RULE_END \
\
RULE(init_declarator_list) \
	VAR SUB(init_declarator) VAR_END() \
	VAR SUB(init_declarator_list) TKN(COMMA) SUB(init_declarator) VAR_END() \
RULE_END \
\
RULE(init_declarator) \
	VAR SUB(declarator) TKN(ASSIGN) SUB(initializer) VAR_END() \
	VAR SUB(declarator) VAR_END() \
RULE_END \
\
RULE(declarator) \
	VAR TKN(IDENT) VAR_END() \
RULE_END \
\
RULE(storage_class_specifier) \
	VAR TKN(EXTERN) VAR_END() \
	VAR TKN(STATIC) VAR_END() \
RULE_END \
\
RULE(func_specifier) \
	VAR TKN(INLINE) VAR_END() \
RULE_END \
\
RULE(type_qualifier) \
	VAR TKN(CONST) VAR_END() \
	VAR TKN(VOLATILE) VAR_END() \
	VAR TKN(ATOMIC) VAR_END() \
RULE_END \
\
RULE(alignment_specifier) \
	VAR TKN(ALIGNAS) TKN(LPAREN) SUB(type) TKN(RPAREN) VAR_END() \
	VAR TKN(ALIGNAS) TKN(LPAREN) SUB(expr) TKN(RPAREN) VAR_END() \
RULE_END \
\
RULE(stmt) \
	VAR SUB(compound_stmt) VAR_END() \
	VAR SUB(expr_stmt) VAR_END() \
RULE_END \
\
RULE(expr_stmt) \
	VAR TKN(SEMICOLON) VAR_END() \
	VAR SUB(expr) TKN(SEMICOLON) VAR_END() \
RULE_END \
\
RULE(initializer) \
	VAR TKN(LBRACE) SUB(initializer_list) TKN(RBRACE) VAR_END() \
	VAR TKN(LBRACE) SUB(initializer_list) TKN(COMMA) TKN(RBRACE) VAR_END() \
	VAR SUB(assignment_expr) VAR_END() \
RULE_END \
\
RULE(initializer_list) \
	VAR SUB(initializer) VAR_END() \
	VAR SUB(initializer_list) TKN(COMMA) SUB(initializer) VAR_END() \
RULE_END \
\
RULE(unit) \
	VAR SUB(import_stmt) VAR_END() \
	VAR SUB(func_def) VAR_END(reduce_unit_1) \
RULE_END \
\
RULE(units) \
	VAR SUB(unit) VAR_END() \
	VAR SUB(units) SUB(unit) VAR_END() \
RULE_END \
\
RULE(import_stmt) \
	VAR TKN(IMPORT) TKN(STRING_LITERAL) TKN(SEMICOLON) VAR_END() \
RULE_END \
\
RULE(func_def) \
	VAR SUB(type) TKN(IDENT) SUB(parameter_list) SUB(compound_stmt) VAR_END(reduce_func_definition) \
RULE_END \
\
RULE(parameter_list) \
	VAR TKN(LPAREN) TKN(RPAREN) VAR_END() \
RULE_END \
\
RULE(type) \
	VAR TKN(VOID) VAR_END() \
	VAR TKN(HASH) TKN(IDENT) VAR_END() \
RULE_END \
\
RULE(compound_stmt) \
	VAR TKN(LBRACE) TKN(RBRACE) VAR_END() \
	VAR TKN(LBRACE) SUB(block_item_list) TKN(RBRACE) VAR_END() \
RULE_END \
\
RULE(block_item_list) \
	VAR SUB(block_item) VAR_END() \
	VAR SUB(block_item_list) SUB(block_item) VAR_END() \
RULE_END \
\
RULE(block_item) \
	/*VAR SUB(declaration) VAR_END()*/ \
	VAR SUB(stmt) VAR_END() \
RULE_END \

// The root node of the grammar.
#define GRAMMAR_ROOT units


// --- REDUCTION ---------------------------------------------------------------

void reduce_unit_1(token_t *red, const token_t *tkn, void *arg) {
	printf("got func_def %p (%s)\n", tkn[0].func_def, tkn[0].func_def->name);
	func_def_dispose(tkn[0].func_def);
}

void reduce_func_definition(token_t *red, const token_t *tkn, void *arg) {
	red->func_def = malloc(sizeof(func_def_t));
	red->func_def->name = strndup(tkn[1].lex.first, tkn[1].lex.last-tkn[1].lex.first);
}
