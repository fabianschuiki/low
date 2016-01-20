/* Copyright (c) 2015-2016 Fabian Schuiki, Thomas Richner */
#include "ast.h"
#include "codegen.h"
#include "codegen_funcs.h"
#include "common.h"
#include <llvm-c/Analysis.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef codegen_context_t context_t;

void
dump_val(char* name,LLVMValueRef val){
	printf("-- %s --:\n",name);
	printf("Value: \t"); fflush(stdout); LLVMDumpValue(val);
	printf("Type: \t"); fflush(stdout); LLVMDumpType(LLVMTypeOf(val));
	printf("--    --\n"); fflush(stdout);
}

void
dump_type(char* name,LLVMTypeRef t){
	printf("-- %s --:\n",name);
	printf("Type: \t"); fflush(stdout); LLVMDumpType(t);
	printf("--    --\n"); fflush(stdout);
}

void
codegen_context_init (codegen_context_t *self) {
	assert(self);
	bzero(self, sizeof *self);
	array_init(&self->symbols, sizeof(codegen_symbol_t));
}

void
codegen_context_dispose (codegen_context_t *self) {
	assert(self);
	array_dispose(&self->symbols);
}

void
codegen_context_add_symbol (codegen_context_t *self, const codegen_symbol_t *symbol) {
	assert(self);
	assert(symbol);
	array_add(&self->symbols, symbol);
}

codegen_symbol_t *
codegen_context_find_symbol (codegen_context_t *self, const char *name) {
	assert(self);
	assert(name);

	unsigned i;
	for (i = 0; i < self->symbols.size; ++i) {
		codegen_symbol_t *symbol = array_get(&self->symbols, i);
		if (symbol->name && strcmp(symbol->name, name) == 0)
			return symbol;
	}

	return self->prev ? codegen_context_find_symbol(self->prev, name) : 0;
}

/// Tries to find the name of the function that implements one of an interface's
/// member functions.
const char *
codegen_context_find_mapping (
	codegen_context_t *self,
	type_t *interface,
	type_t *target,
	const char *name
) {
	assert(self);
	assert(interface);
	assert(target);
	assert(name);

	unsigned i,n;
	for (i = 0; i < self->symbols.size; ++i) {
		codegen_symbol_t *symbol = array_get(&self->symbols, i);
		if (symbol->kind != IMPLEMENTATION_SYMBOL)
			continue;
		if (!type_equal(symbol->decl->impl.interface, interface))
			continue;
		if (!type_equal(symbol->decl->impl.target, target))
			continue;
		for (n = 0; n < symbol->decl->impl.num_mappings; ++n)
			if (strcmp(symbol->decl->impl.mappings[n].intf, name) == 0)
				return symbol->decl->impl.mappings[n].func;
	}

	return self->prev ? codegen_context_find_mapping(self->prev, interface, target, name) : 0;
}

type_t *
resolve_type_name (codegen_context_t *context, type_t *type) {
	assert(type);
	if (type->pointer == 0 && type->kind == AST_NAMED_TYPE) {
		codegen_symbol_t *sym = codegen_context_find_symbol(context, type->name);
		assert(sym && "unknown type name");
		return resolve_type_name(context, sym->type);
	} else {
		return type;
	}
}

void
prepare_expr (codegen_t *self, codegen_context_t *context, expr_t *expr, type_t *type_hint) {
	assert(self);
	assert(context);
	assert(expr);

	prepare_expr_fn_t fn = prepare_expr_fn[expr->kind];
	if (fn) return fn(self, context, expr, type_hint);

	unsigned i;
	switch (expr->kind) {

		case AST_COMMA_EXPR: {
			type_t *type;
			for (i = 0; i < expr->comma.num_exprs; ++i) {
				prepare_expr(self, context, expr->comma.exprs+i, type_hint);
				type = &expr->comma.exprs[i].type;
			}
			type_copy(&expr->type, type);
		} break;

		default:
			die("type determination for expr kind %d not implemented", expr->kind);
	}
}

LLVMValueRef
codegen_expr (codegen_t *self, codegen_context_t *context, expr_t *expr, char lvalue, type_t *type_hint) {
	assert(self);
	assert(context);
	assert(expr);
	/// TODO(fabianschuiki): Split this function up into three functions. One for rvalues that returns the corresponding value, one for lvalues that returns a pointer to the corresponding value, and one for rvalue pointers that returns a pointer to the corresponding value. These will be used for regular expressions, assignments, and struct member accesses respectively.

	codegen_expr_fn_t fn = codegen_expr_fn[expr->kind];
	if (fn) return fn(self, context, expr, lvalue);

	unsigned i;
	switch (expr->kind) {

		case AST_COMMA_EXPR: {
			assert(!lvalue && "comma expression is not a valid lvalue");
			LLVMValueRef value;
			// type_t *type;
			for (i = 0; i < expr->comma.num_exprs; ++i) {
				value = codegen_expr(self, context, &expr->comma.exprs[i], 0, type_hint);
				// type = &expr->comma.exprs[i].type;
			}
			// expr->type = *type;
			return value;
		}

		default:
			die("codegen for expr kind %d not implemented", expr->kind);
			return 0;
	}
}


static LLVMValueRef
codegen_expr_top (codegen_t *self, codegen_context_t *context, expr_t *expr, char lvalue, type_t *type_hint) {
	prepare_expr(self, context, expr, type_hint);
	return codegen_expr(self, context, expr, lvalue, type_hint);
}


static void
codegen_decl (codegen_t *self, codegen_context_t *context, decl_t *decl) {
	assert(self);
	assert(context);
	assert(decl);

	switch(decl->kind) {
		case AST_VARIABLE_DECL: {
			if (decl->variable.type.kind != AST_NO_TYPE) {
				LLVMTypeRef var_type = codegen_type(context, &decl->variable.type);
				LLVMValueRef var = LLVMBuildAlloca(self->builder, var_type, decl->variable.name);

				codegen_symbol_t sym = {
					.name = decl->variable.name,
					.type = &decl->variable.type,
					.decl = decl,
					.value = var,
				};
				codegen_context_add_symbol(context, &sym);

				if (decl->variable.initial) {
					LLVMValueRef val = codegen_expr_top(self, context, decl->variable.initial, 0, &decl->variable.type);
					if (!type_equal(&decl->variable.type, &decl->variable.initial->type)) {
						char *t1 = type_describe(&decl->variable.initial->type);
						char *t2 = type_describe(&decl->variable.type);
						derror(&decl->variable.initial->loc, "initial value of variable '%s' is of type %s, but should be %s\n", decl->variable.name, t1, t2);
						free(t1);
						free(t2);
					}
					LLVMBuildStore(self->builder, val, var);
				} else {
					LLVMBuildStore(self->builder, LLVMConstNull(var_type), var);
				}
			} else {
				LLVMValueRef val = codegen_expr_top(self, context, decl->variable.initial, 0, 0);
				if (decl->variable.initial->type.kind == AST_NO_TYPE)
					derror(&decl->loc, "type of variable '%s' could not be inferred from its initial value\n", decl->variable.name);
				type_copy(&decl->variable.type, &decl->variable.initial->type);

				LLVMValueRef var = LLVMBuildAlloca(self->builder, codegen_type(context, &decl->variable.type), decl->variable.name);
				LLVMBuildStore(self->builder, val, var);

				codegen_symbol_t sym = {
					.name = decl->variable.name,
					.type = &decl->variable.type,
					.decl = decl,
					.value = var,
				};
				codegen_context_add_symbol(context, &sym);
			}
			break;
		}

		case AST_CONST_DECL: {
			codegen_symbol_t sym = {
				.name = decl->cons.name,
				.type = decl->cons.type,
				.decl = decl,
			};

			if (decl->cons.type) {
				LLVMValueRef value = codegen_expr_top(self, context, &decl->cons.value, 0, decl->cons.type);
				LLVMValueRef global = LLVMAddGlobal(self->module, codegen_type(context, decl->cons.type), decl->cons.name);
				LLVMSetInitializer(global, value);
				LLVMSetLinkage(global, LLVMLinkOnceODRLinkage);
				sym.value = global;
			}

			codegen_context_add_symbol(context, &sym);
			break;
		}

		case AST_IMPLEMENTATION_DECL: {
			codegen_symbol_t sym = {
				.kind = IMPLEMENTATION_SYMBOL,
				.decl = decl,
			};

			codegen_context_add_symbol(context, &sym);
			break;
		}

		default:
			die("codegen for decl kind %d not implemented", decl->kind);
	}
}


static void
codegen_stmt (codegen_t *self, codegen_context_t *context, stmt_t *stmt) {
	assert(self);
	assert(context);
	assert(stmt);

	type_t bool_type = { .kind = AST_BOOLEAN_TYPE };
	unsigned i;
	switch(stmt->kind) {

		case AST_EXPR_STMT:
			codegen_expr_top(self, context, stmt->expr, 0, 0);
			break;

		case AST_COMPOUND_STMT: {
			context_t subctx;
			codegen_context_init(&subctx);
			subctx.prev = context;

			for (i = 0; i < stmt->compound.num_items; ++i) {
				const block_item_t *item = stmt->compound.items+i;
				switch (item->kind) {
					case AST_STMT_BLOCK_ITEM:
						if (item->stmt)
							codegen_stmt(self, &subctx, item->stmt);
						break;
					case AST_DECL_BLOCK_ITEM:
						if (item->decl)
							codegen_decl(self, &subctx, item->decl);
						break;
					default:
						fprintf(stderr, "%s.%d: codegen for block_item kind %d not implemented\n", __FILE__, __LINE__, item->kind);
						abort();
						break;
				}
			}

			if (subctx.is_terminated)
				context->is_terminated = 1;
			codegen_context_dispose(&subctx);
			break;
		}

		case AST_IF_STMT: {
			context_t subctx;
			codegen_context_init(&subctx);
			subctx.prev = context;
			LLVMValueRef cond = codegen_expr_top(self, &subctx, stmt->selection.condition, 0, &bool_type);

			if (!type_equal(&stmt->selection.condition->type, &bool_type)) {
				char *td = type_describe(&stmt->selection.condition->type);
				derror(&stmt->loc, "if condition must be a bool expression, got %s instead\n", td);
				free(td);
			}

			LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(self->builder));
			LLVMBasicBlockRef true_block = LLVMAppendBasicBlock(func, "iftrue");
			LLVMBasicBlockRef false_block = stmt->selection.else_stmt ? LLVMAppendBasicBlock(func, "iffalse") : 0;
			LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "ifexit");
			LLVMBuildCondBr(self->builder, cond, true_block, false_block ? false_block : exit_block);

			context_t true_context;
			codegen_context_init(&true_context);
			true_context.prev = context;
			LLVMPositionBuilderAtEnd(self->builder, true_block);
			if (stmt->selection.stmt)
				codegen_stmt(self, &true_context, stmt->selection.stmt);
			if (!true_context.is_terminated)
				LLVMBuildBr(self->builder, exit_block);
			codegen_context_dispose(&true_context);

			int is_terminated = 0;
			if (false_block) {
				context_t false_context;
				codegen_context_init(&false_context);
				false_context.prev = context;
				LLVMPositionBuilderAtEnd(self->builder, false_block);
				codegen_stmt(self, &false_context, stmt->selection.else_stmt);
				if (!false_context.is_terminated)
					LLVMBuildBr(self->builder, exit_block);
				else if (true_context.is_terminated)
					is_terminated = 1;
				codegen_context_dispose(&false_context);
			}

			LLVMPositionBuilderAtEnd(self->builder, exit_block);
			if (is_terminated) {
				LLVMBuildUnreachable(self->builder);
				context->is_terminated = 1;
			}
			codegen_context_dispose(&subctx);
			break;
		}

		case AST_FOR_STMT: {
			context_t subctx;
			codegen_context_init(&subctx);
			subctx.prev = context;
			if (stmt->iteration.initial)
				codegen_expr_top(self, &subctx, stmt->iteration.initial, 0, 0);

			LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(self->builder));
			LLVMBasicBlockRef loop_block = LLVMAppendBasicBlock(func, "loopcond");
			LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(func, "loopbody");
			LLVMBasicBlockRef step_block = LLVMAppendBasicBlock(func, "loopstep");
			LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "loopexit");
			LLVMBuildBr(self->builder, loop_block);

			codegen_t cg = *self;
			cg.continue_block = step_block;
			cg.break_block = exit_block;

			// Check the loop condition.
			LLVMPositionBuilderAtEnd(self->builder, loop_block);
			if (stmt->iteration.condition) {
				LLVMValueRef cond = codegen_expr_top(self, &subctx, stmt->iteration.condition, 0, &bool_type);
				LLVMBuildCondBr(self->builder, cond, body_block, exit_block);
			} else {
				LLVMBuildBr(self->builder, body_block);
			}

			// Execute the loop body.
			LLVMPositionBuilderAtEnd(self->builder, body_block);
			if (stmt->iteration.stmt) {
				codegen_stmt(&cg, &subctx, stmt->iteration.stmt);
			}
			LLVMBuildBr(self->builder, step_block);

			// Execute the loop step.
			LLVMPositionBuilderAtEnd(self->builder, step_block);
			if (stmt->iteration.step) {
				codegen_expr_top(self, &subctx, stmt->iteration.step, 0, 0);
			}
			LLVMBuildBr(self->builder, loop_block);

			LLVMPositionBuilderAtEnd(self->builder, exit_block);
			codegen_context_dispose(&subctx);
			break;
		}

		case AST_CONTINUE_STMT:
			assert(self->continue_block && "nowhere to continue to");
			LLVMBuildBr(self->builder, self->continue_block);
			context->is_terminated = 1;
			break;

		case AST_BREAK_STMT:
			assert(self->break_block && "nowhere to break to");
			LLVMBuildBr(self->builder, self->break_block);
			context->is_terminated = 1;
			break;

		case AST_RETURN_STMT:
			if (stmt->expr) {
				assert(self->unit && self->unit->kind == AST_FUNC_UNIT);
				LLVMBuildRet(self->builder, codegen_expr_top(self, context, stmt->expr, 0, &self->unit->func.return_type));
			} else {
				LLVMBuildRetVoid(self->builder);
			}
			context->is_terminated = 1;
			break;

		default:
			fprintf(stderr, "%s.%d: codegen for stmt kind %d not implemented\n", __FILE__, __LINE__, stmt->kind);
			abort();
			break;
	}
}


static void
codegen_unit (codegen_t *self, codegen_context_t *context, unit_t *unit, int stage) {
	assert(self);
	assert(context);
	assert(unit);

	unsigned i;
	switch (unit->kind) {

		case AST_IMPORT_UNIT:
			break;

		case AST_DECL_UNIT:
			if (stage == 1)
				codegen_decl(self, context, unit->decl);
			break;

		case AST_FUNC_UNIT: {
			LLVMValueRef func;

			if (stage == 1) {
				LLVMTypeRef param_types[unit->func.num_params];
				for (i = 0; i < unit->func.num_params; ++i)
					param_types[i] = codegen_type(context, &unit->func.params[i].type);
				LLVMTypeRef func_type = LLVMFunctionType(codegen_type(context, &unit->func.return_type), param_types, unit->func.num_params, unit->func.variadic);
				LLVMValueRef func = LLVMAddFunction(self->module, unit->func.name, func_type);

				// Declare the function in the context.
				type_t *type = &unit->func.type;
				bzero(type, sizeof(*type));
				type->kind = AST_FUNC_TYPE;
				type->func.return_type = malloc(sizeof(type_t));
				type_copy(type->func.return_type, &unit->func.return_type);
				type->func.num_args = unit->func.num_params;
				type->func.args = malloc(unit->func.num_params * sizeof(type_t));
				for (i = 0; i < unit->func.num_params; ++i) {
					type_copy(type->func.args+i, &unit->func.params[i].type);
				}

				codegen_symbol_t sym = {
					.kind = FUNC_SYMBOL,
					.type = &unit->func.type,
					.name = unit->func.name,
					.value = func,
				};
				codegen_context_add_symbol(context, &sym);
			} else if (stage == 2) {
				codegen_symbol_t *sym = codegen_context_find_symbol(context, unit->func.name);
				assert(sym && "could not find declaration of function");
				func = sym->value;
			}

			// Generate code for the function body.
			if (unit->func.body && stage == 2) {
				LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
				LLVMBuilderRef builder = LLVMCreateBuilder();
				LLVMPositionBuilderAtEnd(builder, block);

				codegen_t cg = *self;
				cg.func = func;
				cg.builder = builder;
				cg.unit = unit;

				context_t subctx;
				codegen_context_init(&subctx);
				subctx.prev = context;

				for (i = 0; i < unit->func.num_params; ++i) {
					func_param_t *param = unit->func.params + i;

					LLVMValueRef param_value = LLVMGetParam(func, i);
					if (param->name)
						LLVMSetValueName(param_value, param->name);

					LLVMValueRef var = LLVMBuildAlloca(builder, codegen_type(context, &param->type), "");
					LLVMBuildStore(builder, param_value, var);

					codegen_symbol_t sym = {
						.name = param->name,
						.type = &param->type,
						.value = var
					};
					codegen_context_add_symbol(&subctx, &sym);
				}

				codegen_stmt(&cg, &subctx, unit->func.body);
				if (!subctx.is_terminated) {
					if (unit->func.return_type.kind == AST_VOID_TYPE)
						LLVMBuildRetVoid(builder);
					else
						derror(&unit->loc, "missing return statement at the end of '%s'\n", unit->func.name);
				}
				codegen_context_dispose(&subctx);

				// LLVMBuildRetVoid(builder);
				LLVMDisposeBuilder(builder);

				// Verify that the function is well-formed.
				LLVMBool failed = LLVMVerifyFunction(func, LLVMPrintMessageAction);
				if (failed) {
					fprintf(stderr, "Function %s contained errors, exiting\n", unit->func.name);
					exit(1);
				}
			}
			break;
		}

		case AST_TYPE_UNIT: {
			if (stage != 0)
				break;
			codegen_symbol_t sym = {
				.name = unit->type.name,
				.type = &unit->type.type,
			};
			codegen_context_add_symbol(context, &sym);
			break;
		}

		default:
			fprintf(stderr, "%s.%d: codegen for unit kind %d not implemented\n", __FILE__, __LINE__, unit->kind);
			abort();
			break;
	}
}


void
codegen_decls (codegen_t *self, codegen_context_t *context, const array_t *units) {
	assert(self);
	assert(context);
	assert(units);

	unsigned i;
	for (i = 0; i < units->size; ++i)
		codegen_unit(self, context, array_get(units,i), 0);
	for (i = 0; i < units->size; ++i)
		codegen_unit(self, context, array_get(units,i), 1);
}


void
codegen_defs (codegen_t *self, codegen_context_t *context, const array_t *units) {
	assert(self);
	assert(context);
	assert(units);

	unsigned i;
	for (i = 0; i < units->size; ++i)
		codegen_unit(self, context, array_get(units,i), 2);
}


void
codegen (codegen_t *self, codegen_context_t *context, const array_t *units) {
	assert(self);
	assert(context);
	assert(units);

	codegen_decls(self, context, units);
	codegen_defs(self, context, units);
}
