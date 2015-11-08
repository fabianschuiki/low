/* Copyright (c) 2015 Fabian Schuiki */
#include "ast.h"
#include "codegen.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct context context_t;
typedef struct local local_t;

struct local {
	const decl_t *decl;
	LLVMValueRef value;
};

struct context {
	context_t *prev;
	array_t locals;
};

static void
context_init (context_t *self) {
	self->prev = 0;
	array_init(&self->locals, sizeof(local_t));
}

static void
context_dispose (context_t *self) {
	array_dispose(&self->locals);
}

static LLVMValueRef
context_find_local (context_t *self, const char *name) {
	assert(self);

	unsigned i;
	for (i = 0; i < self->locals.size; ++i) {
		const local_t *local = array_get(&self->locals, i);
		if (strcmp(local->decl->variable.name, name) == 0)
			return local->value;
	}

	return self->prev ? context_find_local(self->prev, name) : 0;
}


static LLVMTypeRef
codegen_type (const type_t *type) {
	assert(type);
	switch(type->type) {
		case AST_VOID_TYPE:
			return LLVMVoidType();
		case AST_INTEGER_TYPE:
			return LLVMInt32Type();
		case AST_FLOAT_TYPE:
			return LLVMFloatType();
		default:
			fprintf(stderr, "%s.%d: codegen for type %d not implemented\n", __FILE__, __LINE__, type->type);
			abort();
			return 0;
	}
}


static LLVMValueRef
codegen_expr (LLVMBuilderRef builder, context_t *context, const expr_t *expr) {
	assert(expr);
	assert(context);
	switch (expr->type) {
		case AST_IDENT:
			return LLVMBuildLoad(builder, context_find_local(context, expr->ident), "tmp");
			// return LLVMConstNull(LLVMInt32Type());
		case AST_NUMBER_LITERAL:
			return LLVMConstIntOfString(LLVMInt32Type(), expr->number_literal, 10);
		case AST_BINARY_OP: {
			LLVMValueRef lhs = codegen_expr(builder, context, expr->binary_op.lhs);
			LLVMValueRef rhs = codegen_expr(builder, context, expr->binary_op.rhs);
			switch (expr->binary_op.op) {
				case AST_ADD:
					return LLVMBuildAdd(builder, lhs, rhs, "tmp");
				case AST_SUB:
					return LLVMBuildSub(builder, lhs, rhs, "tmp");
				case AST_MUL:
					return LLVMBuildMul(builder, lhs, rhs, "tmp");
				case AST_DIV:
					return LLVMBuildSDiv(builder, lhs, rhs, "tmp");
				default:
					fprintf(stderr, "%s.%d: codegen for binary op %d not implemented\n", __FILE__, __LINE__, expr->binary_op.op);
					abort();
					return 0;
			}
		}

		default:
			fprintf(stderr, "%s.%d: codegen for expr type %d not implemented\n", __FILE__, __LINE__, expr->type);
			abort();
			return 0;
	}
}


static void
codegen_decl (LLVMBuilderRef builder, context_t *context, const decl_t *decl) {
	assert(decl);
	assert(context);
	switch(decl->type) {
		case AST_VARIABLE_DECL: {
			LLVMValueRef var = LLVMBuildAlloca(builder, codegen_type(&decl->variable.type), decl->variable.name);
			local_t local = { .decl = decl, .value = var };
			array_add(&context->locals, &local);
			if (decl->variable.initial)
				LLVMBuildStore(builder, codegen_expr(builder, context, decl->variable.initial), var);
		} break;
		default:
			fprintf(stderr, "%s.%d: codegen for decl type %d not implemented\n", __FILE__, __LINE__, decl->type);
			abort();
			break;
	}
}


static void
codegen_stmt (LLVMBuilderRef builder, context_t *context, const stmt_t *stmt) {
	assert(stmt);
	assert(context);
	unsigned i;
	switch(stmt->type) {
		case AST_COMPOUND_STMT: {
			context_t subcontext;
			context_init(&subcontext);
			subcontext.prev = context;

			for (i = 0; i < stmt->compound.num_items; ++i) {
				const block_item_t *item = stmt->compound.items+i;
				switch (item->type) {
					case AST_STMT_BLOCK_ITEM:
						codegen_stmt(builder, &subcontext, item->stmt);
						break;
					case AST_DECL_BLOCK_ITEM:
						codegen_decl(builder, &subcontext, item->decl);
						break;
					default:
						fprintf(stderr, "%s.%d: codegen for block_item type %d not implemented\n", __FILE__, __LINE__, item->type);
						abort();
						break;
				}
			}

			printf("compound statement had %d locals\n", subcontext.locals.size);
			context_dispose(&subcontext);
		} break;
		case AST_RETURN_STMT:
			if (stmt->expr)
				LLVMBuildRet(builder, codegen_expr(builder, context, stmt->expr));
			else
				LLVMBuildRetVoid(builder);
			break;
		default:
			fprintf(stderr, "%s.%d: codegen for stmt type %d not implemented\n", __FILE__, __LINE__, stmt->type);
			abort();
			break;
	}
}


static void
codegen_unit (LLVMModuleRef module, const unit_t *unit) {
	assert(unit);
	switch (unit->type) {
		case AST_IMPORT_UNIT: {

		} break;

		case AST_FUNC_UNIT: {
			LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), 0, 0, 0);
			LLVMValueRef func = LLVMAddFunction(module, unit->func.name, ret_type);
			LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
			LLVMBuilderRef builder = LLVMCreateBuilder();
			LLVMPositionBuilderAtEnd(builder, block);
			if (unit->func.body) {
				context_t context;
				context_init(&context);
				codegen_stmt(builder, &context, unit->func.body);
				context_dispose(&context);
			}
			// LLVMBuildRetVoid(builder);
			LLVMDisposeBuilder(builder);
		} break;

		default:
			fprintf(stderr, "%s.%d: codegen for unit type %d not implemented\n", __FILE__, __LINE__, unit->type);
			abort();
			break;
	}
}


void
codegen (LLVMModuleRef module, const array_t *units) {
	assert(units);
	unsigned i;
	for (i = 0; i < units->size; ++i) {
		codegen_unit(module, array_get(units, i));
	}
}
