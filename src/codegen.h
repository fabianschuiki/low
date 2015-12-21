/* Copyright (c) 2015 Fabian Schuiki, Thomas Richner */
#pragma once
#include "array.h"
#include "ast.h"
#include <llvm-c/Core.h>

typedef struct codegen codegen_t;
typedef struct codegen_context codegen_context_t;
typedef struct codegen_symbol codegen_symbol_t;

struct codegen {
	LLVMModuleRef module;
	LLVMValueRef func;
	LLVMBuilderRef builder;
	LLVMBasicBlockRef break_block;
	LLVMBasicBlockRef continue_block;
	unit_t *unit;
};

struct codegen_context {
	codegen_context_t *prev;
	array_t symbols;
	unsigned is_terminated;
};

enum codegen_symbol_kind {
	INVALID_SYMBOL = 0,
	FUNC_SYMBOL,
	IMPLEMENTATION_SYMBOL,
	INTERFACE_FUNCTION_SYMBOL,
};

struct codegen_symbol {
	unsigned kind;
	const char *name;
	type_t *type;
	LLVMValueRef value;
	decl_t *decl;
	type_t *interface;
	unsigned member;
};

void codegen_context_init(codegen_context_t *self);
void codegen_context_dispose(codegen_context_t *self);

void codegen_context_add_symbol(codegen_context_t *self, const codegen_symbol_t *symbol);
codegen_symbol_t *codegen_context_find_symbol(codegen_context_t *self, const char *name);

void codegen_decls(codegen_t *self, codegen_context_t *context, const array_t *units);
void codegen_defs(codegen_t *self, codegen_context_t *context, const array_t *units);

void codegen(codegen_t *self, codegen_context_t *context, const array_t *units);
