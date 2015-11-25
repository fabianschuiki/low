/* Copyright (c) 2015 Fabian Schuiki */
#pragma once
#include "grammar.h"

typedef struct assignment_expr assignment_expr_t;
typedef struct binary_expr binary_expr_t;
typedef struct block_item block_item_t;
typedef struct call_expr call_expr_t;
typedef struct cast_expr cast_expr_t;
typedef struct comma_expr comma_expr_t;
typedef struct compound_stmt compound_stmt_t;
typedef struct conditional_expr conditional_expr_t;
typedef struct const_decl const_decl_t;
typedef struct decl decl_t;
typedef struct expr expr_t;
typedef struct func_param func_param_t;
typedef struct func_type func_type_t;
typedef struct func_unit func_unit_t;
typedef struct incdec_expr incdec_expr_t;
typedef struct index_access_expr index_access_expr_t;
typedef struct iteration_stmt iteration_stmt_t;
typedef struct label_stmt label_stmt_t;
typedef struct member_access_expr member_access_expr_t;
typedef struct selection_stmt selection_stmt_t;
typedef struct sizeof_expr sizeof_expr_t;
typedef struct stmt stmt_t;
typedef struct struct_member struct_member_t;
typedef struct struct_type struct_type_t;
typedef struct type type_t;
typedef struct type_unit type_unit_t;
typedef struct unary_expr unary_expr_t;
typedef struct unit unit_t;
typedef struct variable_decl variable_decl_t;



// --- type -------------------------------------------------------------

enum type_kind {
	AST_NO_TYPE,
	AST_VOID_TYPE,
	AST_INTEGER_TYPE,
	AST_FLOAT_TYPE,
	AST_FUNC_TYPE,
	AST_NAMED_TYPE,
	AST_STRUCT_TYPE,
};


struct func_type {
	type_t *return_type;
	unsigned num_args;
	type_t *args;
};


struct struct_type {
	char *name;
	unsigned num_members;
	struct_member_t *members;
};

struct struct_member {
	type_t *type;
	char *name;
};


struct type {
	unsigned kind;
	unsigned pointer;
	union {
		unsigned width;
		func_type_t func;
		struct_type_t strct;
		char *name;
	};
};



// --- expr --------------------------------------------------------------------

enum expr_kind {
	AST_IDENT_EXPR = 0,
	AST_STRING_LITERAL_EXPR,
	AST_NUMBER_LITERAL_EXPR,
	AST_INDEX_ACCESS_EXPR,
	AST_CALL_EXPR,
	AST_MEMBER_ACCESS_EXPR,
	AST_INCDEC_EXPR,
	AST_INITIALIZER_EXPR,
	AST_UNARY_EXPR,
	AST_SIZEOF_EXPR,
	AST_CAST_EXPR,
	AST_BINARY_EXPR,
	AST_CONDITIONAL_EXPR,
	AST_ASSIGNMENT_EXPR,
	AST_COMMA_EXPR,
};


struct index_access_expr {
	expr_t *target;
	expr_t *index;
};


struct call_expr {
	expr_t *target;
	unsigned num_args;
	expr_t *args;
};


struct member_access_expr {
	expr_t *target;
	char *name;
};


enum incdec_order {
	AST_PRE,
	AST_POST,
};

enum incdec_direction {
	AST_INC,
	AST_DEC,
};

struct incdec_expr {
	expr_t *target;
	char order;
	char direction;
};


enum unary_op {
	AST_ADDRESS,
	AST_DEREF,
	AST_POSITIVE,
	AST_NEGATIVE,
	AST_BITWISE_NOT,
	AST_NOT,
};

struct unary_expr {
	expr_t *target;
	unsigned op;
};


enum sizeof_mode {
	AST_EXPR_SIZEOF,
	AST_TYPE_SIZEOF,
};

struct sizeof_expr {
	unsigned mode;
	union {
		expr_t *expr;
		type_t type;
	};
};


struct cast_expr {
	expr_t *target;
	type_t type;
};


enum binary_op {
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

struct binary_expr {
	expr_t *lhs;
	expr_t *rhs;
	unsigned op;
};


struct conditional_expr {
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

struct assignment_expr {
	expr_t *target;
	expr_t *expr;
	unsigned op;
};


struct comma_expr {
	unsigned num_exprs;
	expr_t *exprs;
};


struct expr {
	unsigned kind;
	loc_t loc;
	type_t type;
	union {
		char *ident;
		char *string_literal;
		char *number_literal;
		index_access_expr_t index_access;
		call_expr_t call;
		member_access_expr_t member_access;
		incdec_expr_t incdec_op;
		unary_expr_t unary_op;
		sizeof_expr_t sizeof_op;
		cast_expr_t cast;
		binary_expr_t binary_op;
		conditional_expr_t conditional;
		assignment_expr_t assignment;
		comma_expr_t comma;
	};
};



// --- stmt --------------------------------------------------------------------

enum stmt_kind {
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
	unsigned kind;
	loc_t loc;
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

enum block_item_kind {
	AST_STMT_BLOCK_ITEM,
	AST_DECL_BLOCK_ITEM,
};


struct block_item {
	unsigned kind;
	union {
		stmt_t *stmt;
		decl_t *decl;
	};
};



// --- decl -------------------------------------------------------------

enum decl_kind {
	AST_VARIABLE_DECL,
	AST_CONST_DECL,
	// AST_TYPE_DECL,
	// AST_FUNCTION_DECL,
};


struct variable_decl {
	type_t type;
	char *name;
	expr_t *initial;
};


struct const_decl {
	char *name;
	type_t *type;
	expr_t value;
};


struct decl {
	unsigned kind;
	loc_t loc;
	union {
		variable_decl_t variable;
		const_decl_t cons;
	};
};



// --- unit -------------------------------------------------------------

enum unit_kind {
	AST_IMPORT_UNIT,
	AST_DECL_UNIT,
	AST_FUNC_UNIT,
	AST_TYPE_UNIT,
};


struct func_unit {
	type_t return_type;
	char *name;
	stmt_t *body;
	unsigned num_params;
	func_param_t *params;
	unsigned variadic;
	type_t type;
};

struct func_param {
	type_t type;
	char *name;
};


struct type_unit {
	type_t type;
	char *name;
};


struct unit {
	unsigned kind;
	loc_t loc;
	union {
		char *import_name;
		decl_t *decl;
		func_unit_t func;
		type_unit_t type;
	};
};


void expr_dispose(expr_t *self);
void stmt_dispose(stmt_t *self);
void block_item_dispose(block_item_t *self);
void decl_dispose(decl_t *self);
char *type_describe(type_t *self);
void type_copy(type_t *dst, const type_t *src);
void type_dispose(type_t *self);
void unit_dispose(unit_t *self);
