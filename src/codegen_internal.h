/* Copyright (c) 2015 Fabian Schuiki */
#pragma once
#include "ast.h"
#include "codegen.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREPARE_TYPE(name) void prepare_##name(codegen_t *self, codegen_context_t *context, expr_t *expr, type_t *type_hint)
#define CODEGEN_EXPR(name) LLVMValueRef codegen_##name(codegen_t *self, codegen_context_t *context, expr_t *expr, char lvalue)

typedef void (*prepare_expr_fn_t)(codegen_t *self, codegen_context_t *context, expr_t *expr, type_t *type_hint);
typedef LLVMValueRef (*codegen_expr_fn_t)(codegen_t *self, codegen_context_t *context, expr_t *expr, char lvalue);
