/* Copyright (c) 2015-2016 Fabian Schuiki, Thomas Richner */
#pragma once
#include "ast.h"
#include "codegen.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREPARE_EXPR(name) void prepare_##name(codegen_t *self, codegen_context_t *context, expr_t *expr, type_t *type_hint)
#define CODEGEN_EXPR(name) LLVMValueRef codegen_##name(codegen_t *self, codegen_context_t *context, expr_t *expr, char lvalue)

#define CODEGEN_TYPE(name) LLVMTypeRef codegen_type_##name(codegen_context_t *context, type_t *type)

typedef void (*prepare_expr_fn_t)(codegen_t *self, codegen_context_t *context, expr_t *expr, type_t *type_hint);
typedef LLVMValueRef (*codegen_expr_fn_t)(codegen_t *self, codegen_context_t *context, expr_t *expr, char lvalue);
typedef LLVMTypeRef (*codegen_type_fn_t)(codegen_context_t *context, type_t *type);
