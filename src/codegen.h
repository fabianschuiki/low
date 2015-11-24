/* Copyright (c) 2015 Fabian Schuiki */
#pragma once
#include "array.h"
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
};

struct codegen_context {
	codegen_context_t *prev;
	array_t symbols;
	unsigned is_terminated;
};

struct codegen_symbol {
	const char *name;
	const type_t *type;
	LLVMValueRef value;
};

void codegen_context_init(codegen_context_t *self);
void codegen_context_dispose(codegen_context_t *self);

void codegen_context_add_symbol(codegen_context_t *self, const codegen_symbol_t *symbol);
codegen_symbol_t *codegen_context_find_symbol(codegen_context_t *self, const char *name);

void codegen_decls(codegen_t *self, codegen_context_t *context, const array_t *units);
void codegen_defs(codegen_t *self, codegen_context_t *context, const array_t *units);

void codegen(codegen_t *self, codegen_context_t *context, const array_t *units);
