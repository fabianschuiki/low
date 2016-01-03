/* Copyright (c) 2015 Fabian Schuiki, Thomas Richner */
#pragma once
#include "grammar.h"

#define true 1
#define false 0
typedef unsigned char bool;

typedef struct array_type array_type_t;
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
typedef struct free_builtin free_builtin_t;
typedef struct func_param func_param_t;
typedef struct func_type func_type_t;
typedef struct func_unit func_unit_t;
typedef struct incdec_expr incdec_expr_t;
typedef struct index_access_expr index_access_expr_t;
typedef struct interface_member interface_member_t;
typedef struct interface_member_field interface_member_field_t;
typedef struct interface_member_func interface_member_func_t;
typedef struct interface_type interface_type_t;
typedef struct iteration_stmt iteration_stmt_t;
typedef struct label_stmt label_stmt_t;
typedef struct lencap_builtin lencap_builtin_t;
typedef struct dispose_builtin dispose_builtin_t;
typedef struct make_builtin make_builtin_t;
typedef struct member_access_expr member_access_expr_t;
typedef struct new_builtin new_builtin_t;
typedef struct number_literal number_literal_t;
typedef struct selection_stmt selection_stmt_t;
typedef struct sizeof_expr sizeof_expr_t;
typedef struct slice_type slice_type_t;
typedef struct stmt stmt_t;
typedef struct struct_member struct_member_t;
typedef struct struct_type struct_type_t;
typedef struct type type_t;
typedef struct type_unit type_unit_t;
typedef struct unary_expr unary_expr_t;
typedef struct unit unit_t;
typedef struct variable_decl variable_decl_t;
typedef struct implementation_decl implementation_decl_t;
typedef struct implementation_mapping implementation_mapping_t;
typedef struct index_slice_expr index_slice_expr_t;

// --- type -------------------------------------------------------------

enum type_kind {
	AST_NO_TYPE,
	AST_VOID_TYPE,
	AST_BOOLEAN_TYPE,
	AST_INTEGER_TYPE,
	AST_FLOAT_TYPE,
	AST_FUNC_TYPE,
	AST_NAMED_TYPE,
	AST_STRUCT_TYPE,
	AST_ARRAY_TYPE,
	AST_SLICE_TYPE,
	AST_INTERFACE_TYPE,
	AST_PLACEHOLDER_TYPE,
	AST_NUM_TYPES
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


struct array_type {
	type_t *type;
	unsigned length;
};

struct slice_type {
	type_t *type;
};


struct interface_type {
	unsigned num_members;
	interface_member_t *members;
};

enum interface_member_kind {
	AST_MEMBER_FIELD,
	AST_MEMBER_FUNCTION,
};

struct interface_member_field {
	char *name;
	type_t *type;
};

struct interface_member_func {
	char *name;
	type_t *return_type;
	unsigned num_args;
	type_t *args;
};

struct interface_member {
	unsigned kind;
	union {
		interface_member_field_t field;
		interface_member_func_t func;
	};
};


struct type {
	unsigned kind;
	unsigned pointer;
	union {
		char *name;
		unsigned width;
		func_type_t func;
		struct_type_t strct;
		array_type_t array;
		slice_type_t slice;
		interface_type_t interface;
	};
};



// --- expr --------------------------------------------------------------------

enum expr_kind {
	AST_IDENT_EXPR = 0,
	AST_STRING_LITERAL_EXPR,
	AST_NUMBER_LITERAL_EXPR,
	AST_INDEX_ACCESS_EXPR,
	AST_INDEX_SLICE_EXPR,
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
	AST_NEW_BUILTIN,
	AST_FREE_BUILTIN,
	AST_MAKE_BUILTIN,
	AST_LENCAP_BUILTIN,
	AST_DISPOSE_BUILTIN,
	AST_NUM_EXPRS
};


struct index_access_expr {
	expr_t *target;
	expr_t *index;
};

enum index_slice_kind {
	AST_INDEX_SLICE_RANGE,
	AST_INDEX_SLICE_PREFIX,
	AST_INDEX_SLICE_POSTFIX,
	AST_INDEX_SLICE_COPY,
};

struct index_slice_expr {
	expr_t *target;
	expr_t *start;
	expr_t *end;
	unsigned kind;
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

struct new_builtin {
	type_t type;
};

struct free_builtin {
	expr_t *expr;
};

struct make_builtin {
	type_t type;
	expr_t *expr;
};

enum lencap_kind {
	AST_LEN,
	AST_CAP
};

struct lencap_builtin {
	unsigned kind;
	expr_t *expr;
};

struct dispose_builtin {
	expr_t *expr;
};

struct cast_expr {
	expr_t *target;
	type_t type;
};


enum binary_op {
	AST_ADD,
	AST_SUB,
	AST_MUL,
	AST_DIV,
	AST_MOD,
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

struct number_literal {
	char* literal;
	int radix;
};

struct expr {
	unsigned kind;
	loc_t loc;
	type_t type;
	union {
		char *ident;
		char *string_literal;
		number_literal_t number_literal;
		new_builtin_t newe;
		free_builtin_t free;
		make_builtin_t make;
		index_access_expr_t index_access;
		index_slice_expr_t index_slice;
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
		lencap_builtin_t lencap;
		dispose_builtin_t dispose;
	};
};



// --- stmt --------------------------------------------------------------------

enum stmt_kind {
	AST_EXPR_STMT,
	AST_COMPOUND_STMT,
	AST_IF_STMT,
	AST_SWITCH_STMT,
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
	AST_IMPLEMENTATION_DECL,
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


struct implementation_decl {
	type_t *interface;
	type_t *target;
	unsigned num_mappings;
	implementation_mapping_t *mappings;
};

struct implementation_mapping {
	char *intf;
	char *func;
};


struct decl {
	unsigned kind;
	loc_t loc;
	union {
		variable_decl_t variable;
		const_decl_t cons;
		implementation_decl_t impl;
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
int type_equal(const type_t *a, const type_t *b);
void type_dispose(type_t *self);
void unit_dispose(unit_t *self);
