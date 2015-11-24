/* Copyright (c) 2015 Fabian Schuiki */
#include "ast.h"
#include "codegen.h"
#include <llvm-c/Analysis.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef codegen_context_t context_t;
typedef codegen_symbol_t local_t;


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
	assert(self);

	unsigned i;
	for (i = 0; i < self->symbols.size; ++i) {
		codegen_symbol_t *symbol = array_get(&self->symbols, i);
		if (symbol->name && strcmp(symbol->name, name) == 0)
			return symbol;
	}

	return self->prev ? codegen_context_find_symbol(self->prev, name) : 0;
}


static local_t*
context_find_local (context_t *self, const char *name) {
	assert(self);

	unsigned i;
	for (i = 0; i < self->symbols.size; ++i) {
		local_t *local = array_get(&self->symbols, i);
		// if (local->decl && strcmp(local->decl->variable.name, name) == 0)
		// 	return local;
		if (local->name && strcmp(local->name, name) == 0)
			return local;
	}

	return self->prev ? context_find_local(self->prev, name) : 0;
}


static const type_t *
resolve_type_name (codegen_context_t *context, const type_t *type) {
	assert(type);
	if (type->kind == AST_NAMED_TYPE) {
		codegen_symbol_t *sym = codegen_context_find_symbol(context, type->name);
		assert(sym && "unknown type name");
		return resolve_type_name(context, sym->type);
	} else {
		return type;
	}
}


static LLVMTypeRef
codegen_type (context_t *context, const type_t *type) {
	assert(type);
	if (type->pointer > 0) {
		type_t inner = *type;
		--inner.pointer;
		return LLVMPointerType(codegen_type(context, &inner), 0);
	}
	unsigned i;
	switch(type->kind) {
		case AST_VOID_TYPE:
			return LLVMVoidType();
		case AST_INTEGER_TYPE:
			return LLVMIntType(type->width);
		case AST_FLOAT_TYPE:
			return LLVMFloatType();
		case AST_NAMED_TYPE: {
			local_t *local = context_find_local(context, type->name);
			assert(local && "unknown type name");
			return codegen_type(context, local->type);
		}
		case AST_STRUCT_TYPE: {
			LLVMTypeRef members[type->strct.num_members];
			for (i = 0; i < type->strct.num_members; ++i)
				members[i] = codegen_type(context, type->strct.members[i].type);
			return LLVMStructType(members, type->strct.num_members, 0);
		}
		default:
			fprintf(stderr, "%s.%d: codegen for type kind %d not implemented\n", __FILE__, __LINE__, type->kind);
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
			assert(!lvalue && "number literal is not a valid lvalue");
			expr->type.kind = AST_INTEGER_TYPE;
			expr->type.width = 32;
			return LLVMConstIntOfString(LLVMInt32Type(), expr->number_literal, 10);
		}
		case AST_STRING_LITERAL_EXPR: {
			assert(!lvalue && "string literal is not a valid lvalue");
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
			assert(expr->call.target->kind == AST_IDENT_EXPR && "can only call functions by name at the moment");

			local_t *local = context_find_local(context, expr->call.target->ident);
			if (!local) {
				fprintf(stderr, "identifier '%s' unknown\n", expr->call.target->ident);
				exit(1);
			}
			if (local->type->kind != AST_FUNC_TYPE) {
				fprintf(stderr, "identifier '%s' is not a function\n", local->name);
				exit(1);
			}
			type_copy(&expr->type, local->type->func.return_type);
			LLVMValueRef target = LLVMGetNamedFunction(module, expr->call.target->ident);
			assert(target);
			LLVMValueRef args[expr->call.num_args];
			for (i = 0; i < expr->call.num_args; ++i)
				args[i] = codegen_expr(module, builder, context, &expr->call.args[i], 0);
			LLVMValueRef result = LLVMBuildCall(builder, target, args, expr->call.num_args, "");

			if (lvalue) {
				LLVMValueRef ptr = LLVMBuildAlloca(builder, LLVMTypeOf(result), "");
				LLVMBuildStore(builder, result, ptr);
				return ptr;
			} else {
				return result;
			}
		}

		case AST_MEMBER_ACCESS_EXPR: {
			LLVMValueRef target = codegen_expr(module, builder, context, expr->member_access.target, 1);
			assert(target && "cannot member-access target");
			const type_t *st = resolve_type_name(context, &expr->member_access.target->type);
			assert(st->pointer <= 1 && "cannot access member across multiple indirection");
			LLVMValueRef struct_ptr = st->pointer == 1 ? LLVMBuildLoad(builder, target, "") : target;
			assert(st->kind == AST_STRUCT_TYPE && "cannot access member of non-struct");
			for (i = 0; i < st->strct.num_members; ++i)
				if (strcmp(expr->member_access.name, st->strct.members[i].name) == 0)
					break;
			assert(i < st->strct.num_members && "struct has no such member");
			type_copy(&expr->type, st->strct.members[i].type);
			LLVMValueRef ptr = LLVMBuildStructGEP(builder, struct_ptr, i, "");
			return lvalue ? ptr : LLVMBuildLoad(builder, ptr, "");
			// assert(0 && "not yet implemented");
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
			LLVMValueRef const_one = LLVMConstInt(codegen_type(context, &expr->type), 1, 0);
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
			assert(!lvalue && "result of a cast is not a valid lvalue");
			LLVMValueRef target = codegen_expr(module, builder, context, expr->cast.target, 0);
			expr->type = expr->cast.type;
			LLVMTypeRef type_from = LLVMTypeOf(target);
			LLVMTypeRef type_to = codegen_type(context, &expr->cast.type);
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
			assert(!lvalue && "result of a binary operation is not a valid lvalue");
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

		case AST_CONDITIONAL_EXPR: {
			assert(!lvalue && "result of a conditional is not a valid lvalue");
			LLVMValueRef cond = codegen_expr(module, builder, context, expr->conditional.condition, 0);

			LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
			LLVMBasicBlockRef true_block = LLVMAppendBasicBlock(func, "iftrue");
			LLVMBasicBlockRef false_block = LLVMAppendBasicBlock(func, "iffalse");
			LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "ifexit");
			LLVMBuildCondBr(builder, cond, true_block, false_block ? false_block : exit_block);

			LLVMPositionBuilderAtEnd(builder, true_block);
			LLVMValueRef true_value = codegen_expr(module, builder, context, expr->conditional.true_expr, 0);
			LLVMBuildBr(builder, exit_block);

			LLVMPositionBuilderAtEnd(builder, false_block);
			LLVMValueRef false_value = codegen_expr(module, builder, context, expr->conditional.false_expr, 0);
			LLVMBuildBr(builder, exit_block);

			// TODO: Make sure types of true and false branch match, otherwise cast.
			expr->type = expr->conditional.true_expr->type;

			LLVMPositionBuilderAtEnd(builder, exit_block);
			LLVMValueRef incoming_values[2] = { true_value, false_value };
			LLVMBasicBlockRef incoming_blocks[2] = { true_block, false_block };
			LLVMValueRef phi = LLVMBuildPhi(builder, codegen_type(context, &expr->conditional.true_expr->type), "");
			LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);
			return phi;
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

		case AST_COMMA_EXPR: {
			assert(!lvalue && "comma expression is not a valid lvalue");
			LLVMValueRef value;
			type_t *type;
			for (i = 0; i < expr->comma.num_exprs; ++i) {
				value = codegen_expr(module, builder, context, &expr->comma.exprs[i], 0);
				type = &expr->comma.exprs[i].type;
			}
			expr->type = *type;
			return value;
		}

		default:
			fprintf(stderr, "%s.%d: codegen for expr kind %d not implemented\n", __FILE__, __LINE__, expr->kind);
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
			LLVMValueRef var = LLVMBuildAlloca(builder, codegen_type(context, &decl->variable.type), decl->variable.name);
			local_t local = {
				.name = decl->variable.name,
				.type = &decl->variable.type,
				.value = var
			};
			codegen_context_add_symbol(context, &local);
			if (decl->variable.initial)
				LLVMBuildStore(builder, codegen_expr(module, builder, context, decl->variable.initial, 0), var);
		} break;
		default:
			fprintf(stderr, "%s.%d: codegen for decl kind %d not implemented\n", __FILE__, __LINE__, decl->kind);
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
			codegen_context_init(&subcontext);
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
						fprintf(stderr, "%s.%d: codegen for block_item kind %d not implemented\n", __FILE__, __LINE__, item->kind);
						abort();
						break;
				}
			}

			if (subcontext.is_terminated)
				context->is_terminated = 1;
			codegen_context_dispose(&subcontext);
			break;
		}

		case AST_IF_STMT: {
			context_t subcontext;
			codegen_context_init(&subcontext);
			subcontext.prev = context;
			LLVMValueRef cond = codegen_expr(module, builder, &subcontext, stmt->selection.condition, 0);

			LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
			LLVMBasicBlockRef true_block = LLVMAppendBasicBlock(func, "iftrue");
			LLVMBasicBlockRef false_block = stmt->selection.else_stmt ? LLVMAppendBasicBlock(func, "iffalse") : 0;
			LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "ifexit");
			LLVMBuildCondBr(builder, cond, true_block, false_block ? false_block : exit_block);

			context_t true_context;
			codegen_context_init(&true_context);
			true_context.prev = context;
			LLVMPositionBuilderAtEnd(builder, true_block);
			if (stmt->selection.stmt)
				codegen_stmt(module, builder, &true_context, stmt->selection.stmt);
			if (!true_context.is_terminated)
				LLVMBuildBr(builder, exit_block);
			codegen_context_dispose(&true_context);

			int is_terminated = 0;
			if (false_block) {
				context_t false_context;
				codegen_context_init(&false_context);
				false_context.prev = context;
				LLVMPositionBuilderAtEnd(builder, false_block);
				codegen_stmt(module, builder, &false_context, stmt->selection.else_stmt);
				if (!false_context.is_terminated)
					LLVMBuildBr(builder, exit_block);
				else if (true_context.is_terminated)
					is_terminated = 1;
				codegen_context_dispose(&false_context);
			}

			LLVMPositionBuilderAtEnd(builder, exit_block);
			if (is_terminated) {
				LLVMBuildUnreachable(builder);
				context->is_terminated = 1;
			}
			codegen_context_dispose(&subcontext);
			break;
		}

		case AST_FOR_STMT: {
			context_t subcontext;
			codegen_context_init(&subcontext);
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
			codegen_context_dispose(&subcontext);
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
			fprintf(stderr, "%s.%d: codegen for stmt kind %d not implemented\n", __FILE__, __LINE__, stmt->kind);
			abort();
			break;
	}
}


static void
codegen_unit (LLVMModuleRef module, context_t *context, unit_t *unit, int stage) {
	assert(context);
	assert(unit);
	unsigned i;
	switch (unit->kind) {
		case AST_IMPORT_UNIT:
			break;

		case AST_FUNC_UNIT: {
			LLVMValueRef func;

			if (stage == 1) {
				LLVMTypeRef param_types[unit->func.num_params];
				for (i = 0; i < unit->func.num_params; ++i)
					param_types[i] = codegen_type(context, &unit->func.params[i].type);
				LLVMTypeRef func_type = LLVMFunctionType(codegen_type(context, &unit->func.return_type), param_types, unit->func.num_params, unit->func.variadic);
				LLVMValueRef func = LLVMAddFunction(module, unit->func.name, func_type);

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

				local_t local = {
					.type = &unit->func.type,
					.name = unit->func.name,
					.value = func,
				};
				codegen_context_add_symbol(context, &local);
			} else if (stage == 2) {
				local_t *local = context_find_local(context, unit->func.name);
				assert(local && "could not found declaration of function");
				func = local->value;
			}

			// Generate code for the function body.
			if (unit->func.body && stage == 2) {
				LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
				LLVMBuilderRef builder = LLVMCreateBuilder();
				LLVMPositionBuilderAtEnd(builder, block);

				context_t subcontext;
				codegen_context_init(&subcontext);
				subcontext.prev = context;

				for (i = 0; i < unit->func.num_params; ++i) {
					const func_param_t *param = unit->func.params + i;

					LLVMValueRef param_value = LLVMGetParam(func, i);
					if (param->name)
						LLVMSetValueName(param_value, param->name);

					LLVMValueRef var = LLVMBuildAlloca(builder, codegen_type(context, &param->type), "");
					LLVMBuildStore(builder, param_value, var);

					local_t local = {
						.name = param->name,
						.type = &param->type,
						.value = var
					};
					codegen_context_add_symbol(&subcontext, &local);
				}

				codegen_stmt(module, builder, &subcontext, unit->func.body);
				codegen_context_dispose(&subcontext);

				// LLVMBuildRetVoid(builder);
				LLVMDisposeBuilder(builder);

				// Verify that the function is well-formed.
				LLVMBool failed = LLVMVerifyFunction(func, LLVMPrintMessageAction);
				if (failed) {
					fprintf(stderr, "Function %s contained errors, aborting\n", unit->func.name);
					abort();
				}
			}
			break;
		}

		case AST_TYPE_UNIT: {
			if (stage != 0)
				break;
			local_t local = {
				.name = unit->type.name,
				.type = &unit->type.type,
			};
			codegen_context_add_symbol(context, &local);
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
		codegen_unit(self->module, context, array_get(units,i), 0);
	for (i = 0; i < units->size; ++i)
		codegen_unit(self->module, context, array_get(units,i), 1);
}


void
codegen_defs (codegen_t *self, codegen_context_t *context, const array_t *units) {
	assert(self);
	assert(context);
	assert(units);
	unsigned i;
	for (i = 0; i < units->size; ++i)
		codegen_unit(self->module, context, array_get(units,i), 2);
}


void
codegen (codegen_t *self, codegen_context_t *context, const array_t *units) {
	assert(self);
	assert(context);
	assert(units);

	codegen_decls(self, context, units);
	codegen_defs(self, context, units);
}
