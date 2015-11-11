/* Copyright (c) 2015 Fabian Schuiki */
#pragma once

typedef struct assignment assignment_t;
typedef struct binary_op binary_op_t;
typedef struct block_item block_item_t;
typedef struct call call_t;
typedef struct cast cast_t;
typedef struct comma_expr comma_expr_t;
typedef struct compound_stmt compound_stmt_t;
typedef struct conditional conditional_t;
typedef struct decl decl_t;
typedef struct expr expr_t;
typedef struct func_unit func_unit_t;
typedef struct incdec_op incdec_op_t;
typedef struct index_access index_access_t;
typedef struct iteration_stmt iteration_stmt_t;
typedef struct label_stmt label_stmt_t;
typedef struct member_access member_access_t;
typedef struct selection_stmt selection_stmt_t;
typedef struct sizeof_op sizeof_op_t;
typedef struct stmt stmt_t;
typedef struct type type_t;
typedef struct unary_op unary_op_t;
typedef struct unit unit_t;
typedef struct variable_decl variable_decl_t;



// --- type -------------------------------------------------------------

enum type_type {
	AST_VOID_TYPE,
	AST_INTEGER_TYPE,
	AST_FLOAT_TYPE,
};

struct type {
	unsigned type;
	unsigned width;
	unsigned pointer;
};



// --- expr --------------------------------------------------------------------

enum expr_type {
	AST_IDENT = 0,
	AST_STRING_LITERAL,
	AST_NUMBER_LITERAL,
	AST_INDEX_ACCESS,
	AST_CALL,
	AST_MEMBER_ACCESS,
	AST_INCDEC_OP,
	AST_INITIALIZER,
	AST_UNARY_OP,
	AST_SIZEOF_OP,
	AST_CAST,
	AST_BINARY_OP,
	AST_CONDITIONAL,
	AST_ASSIGNMENT,
	AST_COMMA_EXPR,
};


struct index_access {
	expr_t *target;
	expr_t *index;
};


struct call {
	expr_t *target;
	unsigned num_args;
	expr_t *args;
};


struct member_access {
	expr_t *target;
	char *name;
};


enum incdec_op_order {
	AST_PRE,
	AST_POST,
};

enum incdec_op_direction {
	AST_INC,
	AST_DEC,
};

struct incdec_op {
	expr_t *target;
	char order;
	char direction;
};


enum unary_ops {
	AST_ADDRESS,
	AST_DEREF,
	AST_POSITIVE,
	AST_NEGATIVE,
	AST_BITWISE_NOT,
	AST_NOT,
};

struct unary_op {
	expr_t *target;
	unsigned op;
};


enum sizeof_op_mode {
	AST_SIZEOF_EXPR,
	AST_SIZEOF_TYPE,
};

struct sizeof_op {
	unsigned mode;
	union {
		expr_t *expr;
		void *type;
	};
};


struct cast {
	expr_t *target;
	void *type;
};


enum binary_ops {
	AST_MUL,
	AST_DIV,
	AST_MOD,
	AST_ADD,
	AST_SUB,
	AST_LEFT,
	AST_RIGHT,
	AST_LT,
	AST_GT,
	AST_LE,
	AST_GE,
	AST_EQ,
	AST_NE,
	AST_BITWISE_AND,
	AST_BITWISE_XOR,
	AST_BITWISE_OR,
	AST_AND,
	AST_OR,
};

struct binary_op {
	expr_t *lhs;
	expr_t *rhs;
	unsigned op;
};


struct conditional {
	expr_t *condition;
	expr_t *true_expr;
	expr_t *false_expr;
};


enum assignment_op {
	AST_ASSIGN,
	AST_ADD_ASSIGN,
	AST_AND_ASSIGN,
	AST_DIV_ASSIGN,
	AST_LEFT_ASSIGN,
	AST_MOD_ASSIGN,
	AST_MUL_ASSIGN,
	AST_OR_ASSIGN,
	AST_RIGHT_ASSIGN,
	AST_SUB_ASSIGN,
	AST_XOR_ASSIGN,
};

struct assignment {
	expr_t *target;
	expr_t *expr;
	unsigned op;
};


struct comma_expr {
	unsigned num_exprs;
	expr_t *exprs;
};


struct expr {
	unsigned type;
	union {
		char *ident;
		char *string_literal;
		char *number_literal;
		index_access_t index_access;
		call_t call;
		member_access_t member_access;
		incdec_op_t incdec_op;
		unary_op_t unary_op;
		sizeof_op_t sizeof_op;
		cast_t cast;
		binary_op_t binary_op;
		conditional_t conditional;
		assignment_t assignment;
		comma_expr_t comma_expr;
	};
};



// --- stmt --------------------------------------------------------------------

enum stmt_type {
	AST_EXPR_STMT,
	AST_COMPOUND_STMT,
	AST_IF_STMT,
	AST_SWITCH_STMT,
	AST_WHILE_STMT,
	AST_DO_STMT,
	AST_FOR_STMT,
	AST_GOTO_STMT,
	AST_CONTINUE_STMT,
	AST_BREAK_STMT,
	AST_RETURN_STMT,
	AST_LABEL_STMT,
	AST_CASE_STMT,
	AST_DEFAULT_STMT,
};


struct compound_stmt {
	unsigned num_items;
	block_item_t *items;
};


struct selection_stmt {
	expr_t *condition;
	stmt_t *stmt;
	stmt_t *else_stmt;
};


struct iteration_stmt {
	expr_t *initial;
	expr_t *condition;
	expr_t *step;
	stmt_t *stmt;
};


struct label_stmt {
	stmt_t *stmt;
	union {
		char *name;
		expr_t *expr;
	};
};


struct stmt {
	unsigned type;
	union {
		expr_t *expr;
		compound_stmt_t compound;
		char *name;
		selection_stmt_t selection;
		iteration_stmt_t iteration;
		label_stmt_t label;
	};
};



// --- block_item --------------------------------------------------------------

enum block_item_type {
	AST_STMT_BLOCK_ITEM,
	AST_DECL_BLOCK_ITEM,
};


struct block_item {
	unsigned type;
	union {
		stmt_t *stmt;
		decl_t *decl;
	};
};



// --- decl -------------------------------------------------------------

enum decl_type {
	AST_VARIABLE_DECL,
	// AST_TYPE_DECL,
	// AST_FUNCTION_DECL,
};


struct variable_decl {
	type_t type;
	char *name;
	expr_t *initial;
};


struct decl {
	unsigned type;
	union {
		variable_decl_t variable;
	};
};



// --- unit -------------------------------------------------------------

enum unit_type {
	AST_IMPORT_UNIT,
	AST_DECL_UNIT,
	AST_FUNC_UNIT,
};


struct func_unit {
	type_t return_type;
	char *name;
	stmt_t *body;
};


struct unit {
	unsigned type;
	union {
		char *import_name;
		decl_t *decl;
		func_unit_t func;
	};
};



void expr_dispose(expr_t *self);
void stmt_dispose(stmt_t *self);
void block_item_dispose(block_item_t *self);
void decl_dispose(decl_t *self);
void type_dispose(type_t *self);
void unit_dispose(unit_t *self);
