/* Copyright (c) 2015 Fabian Schuiki */
#include "ast.h"
#include "codegen.h"
#include <llvm-c/Analysis.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct context context_t;
typedef struct local local_t;

struct local {
	const decl_t *decl;
	const type_t *type;
	const char *name;
	LLVMValueRef value;
};

struct context {
	context_t *prev;
	array_t locals;
	unsigned is_terminated;
};

static void
context_init (context_t *self) {
	self->prev = 0;
	self->is_terminated = 0;
	array_init(&self->locals, sizeof(local_t));
}

static void
context_dispose (context_t *self) {
	array_dispose(&self->locals);
}

static local_t*
context_find_local (context_t *self, const char *name) {
	assert(self);

	unsigned i;
	for (i = 0; i < self->locals.size; ++i) {
		local_t *local = array_get(&self->locals, i);
		if (local->decl && strcmp(local->decl->variable.name, name) == 0)
			return local;
		if (local->name && strcmp(local->name, name) == 0)
			return local;
	}

	return self->prev ? context_find_local(self->prev, name) : 0;
}


static LLVMTypeRef
codegen_type (const type_t *type) {
	assert(type);
	if (type->pointer > 0) {
		type_t inner = *type;
		--inner.pointer;
		return LLVMPointerType(codegen_type(&inner), 0);
	}
	switch(type->kind) {
		case AST_VOID_TYPE:
			return LLVMVoidType();
		case AST_INTEGER_TYPE:
			return LLVMIntType(type->width);
		case AST_FLOAT_TYPE:
			return LLVMFloatType();
		default:
			fprintf(stderr, "%s.%d: codegen for type %d not implemented\n", __FILE__, __LINE__, type->kind);
			abort();
			return 0;
	}
}


static LLVMValueRef
codegen_expr (LLVMModuleRef module, LLVMBuilderRef builder, context_t *context, expr_t *expr, char lvalue) {
	assert(expr);
	assert(context);
	unsigned i;
	switch (expr->kind) {
		case AST_IDENT_EXPR: {
			local_t *local = context_find_local(context, expr->ident);
			assert(local && "identifier unknown");
			type_copy(&expr->type, local->type);
			LLVMValueRef ptr = local->value;
			return lvalue ? ptr : LLVMBuildLoad(builder, ptr, "");
		}
		case AST_NUMBER_LITERAL_EXPR: {
			if (lvalue)
				return 0;
			expr->type.kind = AST_INTEGER_TYPE;
			expr->type.width = 32;
			return LLVMConstIntOfString(LLVMInt32Type(), expr->number_literal, 10);
		}
		case AST_STRING_LITERAL_EXPR: {
			if (lvalue)
				return 0;
			expr->type.kind = AST_INTEGER_TYPE;
			expr->type.width = 8;
			expr->type.pointer = 1;
			return LLVMBuildGlobalStringPtr(builder, expr->string_literal, ".str");
		}
		case AST_INDEX_ACCESS_EXPR: {
			LLVMValueRef target = codegen_expr(module, builder, context, expr->index_access.target, 0);
			LLVMValueRef index = codegen_expr(module, builder, context, expr->index_access.index, 0);
			expr->type = expr->index_access.target->type;
			assert(expr->type.pointer > 0 && "cannot index into non-pointer");
			--expr->type.pointer;
			assert(expr->index_access.index->type.kind == AST_INTEGER_TYPE && "index needs to be an integer");
			LLVMValueRef ptr = LLVMBuildInBoundsGEP(builder, target, &index, 1, "");
			return lvalue ? ptr : LLVMBuildLoad(builder, ptr, "");
		}

		case AST_CALL_EXPR: {
			if (lvalue)
				return 0;
			assert(expr->call.target->kind == AST_IDENT_EXPR && "can only call functions by name at the moment");
			expr->type.kind = AST_VOID_TYPE;
			LLVMValueRef target = LLVMGetNamedFunction(module, expr->call.target->ident);
			assert(target);
			LLVMValueRef args[expr->call.num_args];
			for (i = 0; i < expr->call.num_args; ++i)
				args[i] = codegen_expr(module, builder, context, &expr->call.args[i], 0);
			return LLVMBuildCall(builder, target, args, expr->call.num_args, "");
		}

		case AST_INCDEC_EXPR: {
			assert(expr->incdec_op.order == AST_PRE && "post-increment/-decrement not yet implemented");

			LLVMValueRef target = codegen_expr(module, builder, context, expr->incdec_op.target, 1);
			if (!target) {
				fprintf(stderr, "expr %d is not a valid lvalue and thus cannot be incremented/decremented\n", expr->assignment.target->kind);
				abort();
				return 0;
			}
			expr->type = expr->incdec_op.target->type;
			assert(expr->type.kind == AST_INTEGER_TYPE && "increment/decrement operator only supported for integers");

			LLVMValueRef value = LLVMBuildLoad(builder, target, "");
			LLVMValueRef const_one = LLVMConstInt(codegen_type(&expr->type), 1, 0);
			LLVMValueRef new_value;
			if (expr->incdec_op.direction == AST_INC) {
				new_value = LLVMBuildAdd(builder, value, const_one, "");
			} else {
				new_value = LLVMBuildSub(builder, value, const_one, "");
			}

			LLVMBuildStore(builder, new_value, target);
			return lvalue ? target : new_value;
		}

		case AST_UNARY_EXPR: {
			LLVMValueRef target = codegen_expr(module, builder, context, expr->unary_op.target, 0);
			switch (expr->unary_op.op) {
				case AST_DEREF:
					expr->type = expr->unary_op.target->type;
					assert(expr->type.pointer > 0 && "cannot dereference non-pointer");
					--expr->type.pointer;
					return lvalue ? target : LLVMBuildLoad(builder, target, "");
				default:
					fprintf(stderr, "%s.%d: codegen for unary op %d not implemented\n", __FILE__, __LINE__, expr->unary_op.op);
					abort();
					return 0;
			}
		}

		case AST_CAST_EXPR: {
			if (lvalue)
				return 0;
			LLVMValueRef target = codegen_expr(module, builder, context, expr->cast.target, 0);
			expr->type = expr->cast.type;
			LLVMTypeRef type_from = LLVMTypeOf(target);
			LLVMTypeRef type_to = codegen_type(&expr->cast.type);
			LLVMTypeKind kind_from = LLVMGetTypeKind(type_from);
			LLVMTypeKind kind_to = LLVMGetTypeKind(type_to);
			switch (kind_from) {
				case LLVMIntegerTypeKind: {
					switch (kind_to) {
						case LLVMIntegerTypeKind:
							return LLVMBuildIntCast(builder, target, type_to, "");
						case LLVMPointerTypeKind:
							return LLVMBuildIntToPtr(builder, target, type_to, "");
						default: break;
					}
					break;
				}
				case LLVMPointerTypeKind: {
					if (kind_to == LLVMPointerTypeKind)
						return LLVMBuildPointerCast(builder, target, type_to, "");
					break;
				}
				default: break;
			}

			char *str_from = LLVMPrintTypeToString(type_from);
			char *str_to = LLVMPrintTypeToString(type_to);
			fprintf(stderr, "cannot cast from %s to %s\n", str_from, str_to);
			free(str_from);
			free(str_to);
			abort();
			return 0;
		}

		case AST_BINARY_EXPR: {
			if (lvalue)
				return 0;
			LLVMValueRef lhs = codegen_expr(module, builder, context, expr->binary_op.lhs, 0);
			LLVMValueRef rhs = codegen_expr(module, builder, context, expr->binary_op.rhs, 0);
			assert(expr->binary_op.lhs->type.kind == expr->binary_op.rhs->type.kind &&
			       expr->binary_op.lhs->type.width == expr->binary_op.rhs->type.width &&
			       expr->binary_op.lhs->type.pointer == expr->binary_op.rhs->type.pointer &&
			       "operands to binary operator must be of the same type");
			expr->type = expr->assignment.expr->type;
			switch (expr->binary_op.op) {
				case AST_ADD:
					return LLVMBuildAdd(builder, lhs, rhs, "");
				case AST_SUB:
					return LLVMBuildSub(builder, lhs, rhs, "");
				case AST_MUL:
					return LLVMBuildMul(builder, lhs, rhs, "");
				case AST_DIV:
					return LLVMBuildSDiv(builder, lhs, rhs, "");
				case AST_LT:
					return LLVMBuildICmp(builder, LLVMIntSLT, lhs, rhs, "");
				case AST_GT:
					return LLVMBuildICmp(builder, LLVMIntSGT, lhs, rhs, "");
				case AST_LE:
					return LLVMBuildICmp(builder, LLVMIntSLE, lhs, rhs, "");
				case AST_GE:
					return LLVMBuildICmp(builder, LLVMIntSGE, lhs, rhs, "");
				case AST_EQ:
					return LLVMBuildICmp(builder, LLVMIntEQ, lhs, rhs, "");
				case AST_NE:
					return LLVMBuildICmp(builder, LLVMIntNE, lhs, rhs, "");
				default:
					fprintf(stderr, "%s.%d: codegen for binary op %d not implemented\n", __FILE__, __LINE__, expr->binary_op.op);
					abort();
					return 0;
			}
		}

		case AST_ASSIGNMENT_EXPR: {
			LLVMValueRef lv = codegen_expr(module, builder, context, expr->assignment.target, 1);
			if (!lv) {
				fprintf(stderr, "expr %d cannot be assigned a value\n", expr->assignment.target->kind);
				abort();
				return 0;
			}
			LLVMValueRef rv = codegen_expr(module, builder, context, expr->assignment.expr, 0);
			assert(expr->assignment.target->type.kind == expr->assignment.expr->type.kind &&
			       expr->assignment.target->type.width == expr->assignment.expr->type.width &&
			       expr->assignment.target->type.pointer == expr->assignment.expr->type.pointer &&
			       "incompatible types in assignment");
			expr->type = expr->assignment.expr->type;
			LLVMValueRef v;
			if (expr->assignment.op == AST_ASSIGN) {
				v = rv;
			} else {
				LLVMValueRef dv = LLVMBuildLoad(builder, lv, "");
				switch (expr->assignment.op) {
					case AST_ADD_ASSIGN: v = LLVMBuildAdd(builder, dv, rv, ""); break;
					case AST_SUB_ASSIGN: v = LLVMBuildSub(builder, dv, rv, ""); break;
					case AST_MUL_ASSIGN: v = LLVMBuildMul(builder, dv, rv, ""); break;
					case AST_DIV_ASSIGN: v = LLVMBuildUDiv(builder, dv, rv, ""); break;
					default:
						fprintf(stderr, "%s.%d: codegen for assignment op %d not implemented\n", __FILE__, __LINE__, expr->assignment.op);
						abort();
						return 0;
				}
			}
			LLVMBuildStore(builder, v, lv);
			return rv;
		}

		default:
			fprintf(stderr, "%s.%d: codegen for expr type %d not implemented\n", __FILE__, __LINE__, expr->kind);
			abort();
			return 0;
	}
}


static void
codegen_decl (LLVMModuleRef module, LLVMBuilderRef builder, context_t *context, const decl_t *decl) {
	assert(decl);
	assert(context);
	switch(decl->kind) {
		case AST_VARIABLE_DECL: {
			LLVMValueRef var = LLVMBuildAlloca(builder, codegen_type(&decl->variable.type), decl->variable.name);
			local_t local = {
				.decl = decl,
				.type = &decl->variable.type,
				.value = var
			};
			array_add(&context->locals, &local);
			if (decl->variable.initial)
				LLVMBuildStore(builder, codegen_expr(module, builder, context, decl->variable.initial, 0), var);
		} break;
		default:
			fprintf(stderr, "%s.%d: codegen for decl type %d not implemented\n", __FILE__, __LINE__, decl->kind);
			abort();
			break;
	}
}


static void
codegen_stmt (LLVMModuleRef module, LLVMBuilderRef builder, context_t *context, const stmt_t *stmt) {
	assert(stmt);
	assert(context);
	unsigned i;
	switch(stmt->kind) {
		case AST_EXPR_STMT:
			codegen_expr(module, builder, context, stmt->expr, 0);
			break;
		case AST_COMPOUND_STMT: {
			context_t subcontext;
			context_init(&subcontext);
			subcontext.prev = context;

			for (i = 0; i < stmt->compound.num_items; ++i) {
				const block_item_t *item = stmt->compound.items+i;
				switch (item->kind) {
					case AST_STMT_BLOCK_ITEM:
						codegen_stmt(module, builder, &subcontext, item->stmt);
						break;
					case AST_DECL_BLOCK_ITEM:
						codegen_decl(module, builder, &subcontext, item->decl);
						break;
					default:
						fprintf(stderr, "%s.%d: codegen for block_item type %d not implemented\n", __FILE__, __LINE__, item->kind);
						abort();
						break;
				}
			}

			if (subcontext.is_terminated)
				context->is_terminated = 1;
			context_dispose(&subcontext);
			break;
		}

		case AST_IF_STMT: {
			context_t subcontext;
			context_init(&subcontext);
			subcontext.prev = context;
			LLVMValueRef cond = codegen_expr(module, builder, &subcontext, stmt->selection.condition, 0);

			LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
			LLVMBasicBlockRef true_block = LLVMAppendBasicBlock(func, "iftrue");
			LLVMBasicBlockRef false_block = stmt->selection.else_stmt ? LLVMAppendBasicBlock(func, "iffalse") : 0;
			LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "ifexit");
			LLVMBuildCondBr(builder, cond, true_block, false_block ? false_block : exit_block);

			context_t true_context;
			context_init(&true_context);
			true_context.prev = context;
			LLVMPositionBuilderAtEnd(builder, true_block);
			if (stmt->selection.stmt)
				codegen_stmt(module, builder, &true_context, stmt->selection.stmt);
			if (!true_context.is_terminated)
				LLVMBuildBr(builder, exit_block);
			context_dispose(&true_context);

			if (false_block) {
				context_t false_context;
				context_init(&false_context);
				false_context.prev = context;
				LLVMPositionBuilderAtEnd(builder, false_block);
				codegen_stmt(module, builder, &false_context, stmt->selection.else_stmt);
				if (!false_context.is_terminated)
					LLVMBuildBr(builder, exit_block);
				context_dispose(&false_context);
			}

			LLVMPositionBuilderAtEnd(builder, exit_block);
			context_dispose(&subcontext);
			break;
		}

		case AST_FOR_STMT: {
			context_t subcontext;
			context_init(&subcontext);
			subcontext.prev = context;
			codegen_expr(module, builder, &subcontext, stmt->iteration.initial, 0);

			LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
			LLVMBasicBlockRef loop_block = LLVMAppendBasicBlock(func, "loopinit");
			LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(func, "loopbody");
			LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "loopexit");
			LLVMBuildBr(builder, loop_block);

			LLVMPositionBuilderAtEnd(builder, loop_block);
			LLVMValueRef cond = codegen_expr(module, builder, &subcontext, stmt->iteration.condition, 0);
			LLVMBuildCondBr(builder, cond, body_block, exit_block);

			LLVMPositionBuilderAtEnd(builder, body_block);
			if (stmt->iteration.stmt)
				codegen_stmt(module, builder, &subcontext, stmt->iteration.stmt);
			if (stmt->iteration.step)
				codegen_expr(module, builder, &subcontext, stmt->iteration.step, 0);
			LLVMBuildBr(builder, loop_block);

			LLVMPositionBuilderAtEnd(builder, exit_block);
			context_dispose(&subcontext);
			break;
		}

		case AST_RETURN_STMT:
			if (stmt->expr)
				LLVMBuildRet(builder, codegen_expr(module, builder, context, stmt->expr, 0));
			else
				LLVMBuildRetVoid(builder);
			context->is_terminated = 1;
			break;
		default:
			fprintf(stderr, "%s.%d: codegen for stmt type %d not implemented\n", __FILE__, __LINE__, stmt->kind);
			abort();
			break;
	}
}


static void
codegen_unit (LLVMModuleRef module, const unit_t *unit) {
	assert(unit);
	unsigned i;
	switch (unit->kind) {
		case AST_IMPORT_UNIT: {

		} break;

		case AST_FUNC_UNIT: {
			LLVMTypeRef param_types[unit->func.num_params];
			for (i = 0; i < unit->func.num_params; ++i)
				param_types[i] = codegen_type(&unit->func.params[i].type);
			LLVMTypeRef func_type = LLVMFunctionType(codegen_type(&unit->func.return_type), param_types, unit->func.num_params, unit->func.variadic);
			LLVMValueRef func = LLVMAddFunction(module, unit->func.name, func_type);

			if (unit->func.body) {
				LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
				LLVMBuilderRef builder = LLVMCreateBuilder();
				LLVMPositionBuilderAtEnd(builder, block);

				context_t context;
				context_init(&context);

				for (i = 0; i < unit->func.num_params; ++i) {
					const func_param_t *param = unit->func.params + i;

					LLVMValueRef param_value = LLVMGetParam(func, i);
					if (param->name)
						LLVMSetValueName(param_value, param->name);

					LLVMValueRef var = LLVMBuildAlloca(builder, codegen_type(&param->type), "");
					LLVMBuildStore(builder, param_value, var);

					local_t local = {
						.name = param->name,
						.type = &param->type,
						.value = var
					};
					array_add(&context.locals, &local);
				}

				codegen_stmt(module, builder, &context, unit->func.body);
				context_dispose(&context);

				// LLVMBuildRetVoid(builder);
				LLVMDisposeBuilder(builder);

				// Verify that the function is well-formed.
				LLVMBool failed = LLVMVerifyFunction(func, LLVMPrintMessageAction);
				if (failed) {
					fprintf(stderr, "Function %s contained errors, aborting\n", unit->func.name);
					abort();
				}
			}
		} break;

		default:
			fprintf(stderr, "%s.%d: codegen for unit type %d not implemented\n", __FILE__, __LINE__, unit->kind);
			abort();
			break;
	}
}


void
codegen (LLVMModuleRef module, const array_t *units) {
	assert(units);

	// LLVMTypeRef func_args[1] = { LLVMPointerType(LLVMInt8Type(), 0) };
	// LLVMTypeRef func_type = LLVMFunctionType(LLVMInt32Type(), func_args, 1, 0);
	// LLVMAddFunction(module, "puts", func_type);

	unsigned i;
	for (i = 0; i < units->size; ++i) {
		codegen_unit(module, array_get(units, i));
	}
}
