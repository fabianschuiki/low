/* Copyright (c) 2015 Fabian Schuiki */
#define GRAMMAR \
\
RULE(unit) \
	VAR SUB(import_statement) VAR_END() \
	VAR SUB(func_definition) VAR_END() \
RULE_END \
\
RULE(units) \
	VAR SUB(unit) VAR_END() \
	VAR SUB(units) SUB(unit) VAR_END() \
RULE_END \
\
RULE(import_statement) \
	VAR TKN(IMPORT) TKN(STRING_LITERAL) TKN(SEMICOLON) VAR_END() \
RULE_END \
\
RULE(func_definition) \
	VAR SUB(type) TKN(IDENT) TKN(LPAREN) TKN(RPAREN) SUB(compound_statement) VAR_END() \
RULE_END \
\
RULE(type) \
	VAR TKN(VOID) VAR_END() \
	VAR TKN(IDENT) VAR_END() \
RULE_END \
\
RULE(compound_statement) \
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
	VAR SUB(statement) VAR_END() \
RULE_END \
\
RULE(statement) \
	VAR SUB(compound_statement) VAR_END() \
	VAR TKN(SEMICOLON) VAR_END() \
RULE_END \

// The root node of the grammar.
#define GRAMMAR_ROOT units
