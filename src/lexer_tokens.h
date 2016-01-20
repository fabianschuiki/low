/* Copyright (c) 2015 Fabian Schuiki, Thomas Richner */
#define TOKENS \
/* literals */ \
TKN(IDENT, "identifier") \
TKN(NUMBER_LITERAL, "number literal") \
TKN(STRING_LITERAL, "string literal") \
\
/* operators */ \
TKN(ADD_OP, "+") \
TKN(AND_OP, "&&") \
TKN(BITWISE_AND, "&") \
TKN(BITWISE_NOT, "~") \
TKN(BITWISE_OR, "|") \
TKN(BITWISE_XOR, "^") \
TKN(DEC_OP, "--") \
TKN(DIV_OP, "/") \
TKN(EQ_OP, "=") \
TKN(GE_OP, ">=") \
TKN(GT_OP, ">") \
TKN(INC_OP, "++") \
TKN(LEFT_OP, "<<") \
TKN(LE_OP, "<=") \
TKN(LT_OP, "<") \
TKN(MAPTO, "->") \
TKN(MOD_OP, "%") \
TKN(MUL_OP, "*") \
TKN(NE_OP, "!=") \
TKN(NOT_OP, "!") \
TKN(OR_OP, "||") \
TKN(RIGHT_OP, ">>") \
TKN(SUB_OP, "-") \
\
/* assignment operators */ \
TKN(ASSIGN, "=") \
TKN(ADD_ASSIGN, "+=") \
TKN(AND_ASSIGN, "&=") \
TKN(DEF_ASSIGN, ":=") \
TKN(DIV_ASSIGN, "/=") \
TKN(LEFT_ASSIGN, "<<=") \
TKN(MOD_ASSIGN, "%=") \
TKN(MUL_ASSIGN, "*=") \
TKN(OR_ASSIGN, "|=") \
TKN(RIGHT_ASSIGN, ">>=") \
TKN(SUB_ASSIGN, "-=") \
TKN(XOR_ASSIGN, "^=") \
\
/* symbols */ \
TKN(COMMA, ",") \
TKN(COLON, ":") \
TKN(ELLIPSIS, "...") \
TKN(HASH, "#") \
TKN(LBRACE, "{") \
TKN(LBRACK, "[") \
TKN(LPAREN, "(") \
TKN(PERIOD, ".") \
TKN(QUESTION, "?") \
TKN(RBRACE, "}") \
TKN(RBRACK, "]") \
TKN(RPAREN, ")") \
TKN(SEMICOLON, ";") \
\
/* keywords */ \
TKN(ALIGNAS, "alignas") \
TKN(ATOMIC, "atomic") \
TKN(BREAK, "break") \
TKN(CASE, "case") \
TKN(CONST, "const") \
TKN(CONTINUE, "continue") \
TKN(DEFAULT, "default") \
TKN(DO, "do") \
TKN(ELSE, "else") \
TKN(ENUM, "enum") \
TKN(EXTERN, "extern") \
TKN(FOR, "for") \
TKN(FUNC, "func") \
TKN(GOTO, "goto") \
TKN(IF, "if") \
TKN(IMPORT, "import") \
TKN(INLINE, "inline") \
TKN(INTERFACE, "interface") \
TKN(NEW, "new") \
TKN(FREE, "free") \
TKN(MAKE, "make") \
TKN(LEN, "len") \
TKN(CAP, "cap") \
TKN(PACKAGE, "package") \
TKN(DISPOSE, "dispose") \
TKN(RETURN, "return") \
TKN(SIZEOF, "sizeof") \
TKN(STATIC, "static") \
TKN(STRUCT, "struct") \
TKN(SWITCH, "switch") \
TKN(TYPE, "type") \
TKN(TYPEDEF, "typedef") \
TKN(UNION, "union") \
TKN(VAR, "var") \
TKN(VOID, "void") \
TKN(VOLATILE, "volatile") \
TKN(WHILE, "while") \
TKN(YIELD, "yield")
