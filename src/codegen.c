/* Copyright (c) 2015 Fabian Schuiki */
#include "ast.h"
#include "codegen.h"
#include <llvm-c/Analysis.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef codegen_context_t context_t;


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


static codegen_symbol_t*
context_find_local (context_t *self, const char *name) {
	assert(self);

	unsigned i;
	for (i = 0; i < self->symbols.size; ++i) {
		codegen_symbol_t *local = array_get(&self->symbols, i);
		// if (local->decl && strcmp(local->decl->variable.name, name) == 0)
		// 	return local;
		if (local->name && strcmp(local->name, name) == 0)
			return local;
	}

	return self->prev ? context_find_local(self->prev, name) : 0;
}


static type_t *
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


static LLVMTypeRef
codegen_type (context_t *context, const type_t *type) {
	assert(type);
	if (type->pointer > 0) {
		type_t inner = *type;
		--inner.pointer;
		return LLVMPointerType(codegen_type(context, &inner), 0);
	}
	unsigned i;
	switch (type->kind) {
		case AST_VOID_TYPE:
			return LLVMVoidType();
		case AST_BOOLEAN_TYPE:
			return LLVMInt1Type();
		case AST_INTEGER_TYPE:
			return LLVMIntType(type->width);
		case AST_FLOAT_TYPE:
			switch (type->width) {
				case 16: return LLVMHalfType();
				case 32: return LLVMFloatType();
				case 64: return LLVMDoubleType();
				case 128: return LLVMFP128Type();
				default:
					derror(0, "floating point type must be 16, 32, 64 or 128 bits wide\n");
			}
		case AST_FUNC_TYPE: {
			LLVMTypeRef args[type->func.num_args];
			for (i = 0; i < type->func.num_args; ++i)
				args[i] = codegen_type(context, &type->func.args[i]);
			return LLVMFunctionType(codegen_type(context, type->func.return_type), args, type->func.num_args, 0);
		}
		case AST_NAMED_TYPE: {
			codegen_symbol_t *sym = context_find_local(context, type->name);
			if (!sym)
				derror(0, "unknown type name '%s'\n", type->name);
			if (!sym->type)
				derror(0, "'%s' is not a type name\n", type->name);
			// assert(sym && "unknown type name");
			return codegen_type(context, sym->type);
		}
		case AST_STRUCT_TYPE: {
			LLVMTypeRef members[type->strct.num_members];
			for (i = 0; i < type->strct.num_members; ++i)
				members[i] = codegen_type(context, type->strct.members[i].type);
			return LLVMStructType(members, type->strct.num_members, 0);
		}
		case AST_ARRAY_TYPE: {
			LLVMTypeRef element = codegen_type(context, type->array.type);
			return LLVMArrayType(element, type->array.length);
		}
		default:
			fprintf(stderr, "%s.%d: codegen for type kind %d not implemented\n", __FILE__, __LINE__, type->kind);
			abort();
			return 0;
	}
}


static void
determine_type (codegen_t *self, codegen_context_t *context, expr_t *expr, type_t *type_hint) {
	assert(self);
	assert(context);
	assert(expr);

	unsigned i;
	switch (expr->kind) {

		case AST_IDENT_EXPR: {
			if (strcmp(expr->ident, "true") == 0 || strcmp(expr->ident, "false") == 0) {
				expr->type.kind = AST_BOOLEAN_TYPE;
				break;
			}

			codegen_symbol_t *sym = codegen_context_find_symbol(context, expr->ident);
			if (!sym)
				derror(&expr->loc, "identifier '%s' unknown\n", expr->ident);

			if (sym->kind == FUNC_SYMBOL) {
				type_copy(&expr->type, sym->type);
				++expr->type.pointer;
			} else if (sym->value) {
				type_copy(&expr->type, sym->type);
			} else {
				if (!sym->decl || sym->decl->kind != AST_CONST_DECL)
					derror(&expr->loc, "expected identifier '%s' to be a const", expr->ident);
				determine_type(self, context, &sym->decl->cons.value, type_hint);
				type_copy(&expr->type, &sym->decl->cons.value.type);
			}
		} break;

		case AST_NUMBER_LITERAL_EXPR: {
			if (type_hint)
				type_copy(&expr->type, type_hint);
		} break;

		case AST_STRING_LITERAL_EXPR: {
			expr->type.kind = AST_INTEGER_TYPE;
			expr->type.width = 8;
			expr->type.pointer = 1;
		} break;

		case AST_INDEX_ACCESS_EXPR: {
			type_t int_type = { .kind = AST_INTEGER_TYPE, .pointer = 0, .width = 64 };
			determine_type(self, context, expr->index_access.target, 0);
			determine_type(self, context, expr->index_access.index, &int_type);
			type_t *target = &expr->index_access.target->type;
			if (target->pointer > 0) {
				type_copy(&expr->type, target);
				--expr->type.pointer;
			} else if (target->kind == AST_ARRAY_TYPE) {
				type_copy(&expr->type, target->array.type);
			} else {
				derror(&expr->loc, "cannot index into non-pointer\n");
			}
		} break;

		case AST_CALL_EXPR: {
			assert(expr->call.target->kind == AST_IDENT_EXPR && "can only call functions by name at the moment");
			codegen_symbol_t *sym = codegen_context_find_symbol(context, expr->call.target->ident);
			if (!sym)
				derror(&expr->loc, "identifier '%s' unknown\n", expr->call.target->ident);
			if (sym->type->kind != AST_FUNC_TYPE)
				derror(&expr->loc, "identifier '%s' is not a function\n", sym->name);
			type_copy(&expr->type, sym->type->func.return_type);
			for (i = 0; i < expr->call.num_args; ++i)
				determine_type(self, context, expr->call.args+i, sym->type->func.args+i);
		} break;

		case AST_MEMBER_ACCESS_EXPR: {
			determine_type(self, context, expr->member_access.target, 0);
			type_t *st = resolve_type_name(context, &expr->member_access.target->type);
			type_t tmp;
			if (st->pointer > 0) {
				tmp = *st;
				--tmp.pointer;
				st = resolve_type_name(context, &tmp);
			}
			if (st->kind != AST_STRUCT_TYPE)
				derror(&expr->loc, "cannot access member of non-struct\n");
			for (i = 0; i < st->strct.num_members; ++i)
				if (strcmp(expr->member_access.name, st->strct.members[i].name) == 0)
					break;
			if (i == st->strct.num_members)
				derror(&expr->loc, "struct has no member named '%s'\n", expr->member_access.name);
			type_copy(&expr->type, st->strct.members[i].type);
		} break;

		case AST_INCDEC_EXPR: {
			determine_type(self, context, expr->incdec_op.target, type_hint);
			type_copy(&expr->type, &expr->incdec_op.target->type);
		} break;

		case AST_UNARY_EXPR: {
			switch (expr->unary_op.op) {
				case AST_ADDRESS: {
					type_t *inner_hint = 0;
					type_t hint;
					if (type_hint && type_hint->pointer > 0) {
						type_copy(&hint, type_hint);
						--hint.pointer;
						inner_hint = &hint;
					}
					determine_type(self, context, expr->unary_op.target, inner_hint);
					type_copy(&expr->type, &expr->unary_op.target->type);
					++expr->type.pointer;
				} break;

				case AST_DEREF: {
					if (type_hint) {
						type_t new_hint;
						type_copy(&new_hint, type_hint);
						++new_hint.pointer;
						determine_type(self, context, expr->unary_op.target, &new_hint);
					} else {
						determine_type(self, context, expr->unary_op.target, 0);
					}
					type_copy(&expr->type, &expr->unary_op.target->type);
					expr->type = expr->unary_op.target->type;
					if (expr->type.pointer == 0)
						derror(&expr->loc, "cannot dereference non-pointer\n");
					--expr->type.pointer;
				} break;

				case AST_POSITIVE:
				case AST_NEGATIVE:
				case AST_BITWISE_NOT: {
					determine_type(self, context, expr->unary_op.target, type_hint);
					type_copy(&expr->type, &expr->unary_op.target->type);
				} break;

				case AST_NOT: {
					type_t bool_type = { .kind = AST_BOOLEAN_TYPE };
					determine_type(self, context, expr->unary_op.target, &bool_type);
					bzero(&expr->type, sizeof expr->type);
					expr->type.kind = AST_BOOLEAN_TYPE;
				} break;

				default:
					fprintf(stderr, "%s.%d: type determination for unary op %d not implemented\n", __FILE__, __LINE__, expr->unary_op.op);
					abort();
			}
		} break;

		case AST_SIZEOF_EXPR: {
			if (expr->sizeof_op.mode == AST_EXPR_SIZEOF)
				determine_type(self, context, expr->sizeof_op.expr, 0);
			if (type_hint)
				type_copy(&expr->type, type_hint);
		} break;

		case AST_NEW_BUILTIN: {
			type_copy(&expr->type, &expr->newe.type);
			++expr->type.pointer;
		} break;

		case AST_FREE_BUILTIN: {
			determine_type(self, context, expr->free.expr, 0);
			expr->type.kind = AST_VOID_TYPE;

			if (expr->free.expr->type.pointer == 0) {
				derror(&expr->loc, "cannot free non-pointer\n");
			}
		} break;

		case AST_CAST_EXPR: {
			determine_type(self, context, expr->cast.target, &expr->cast.type);
			type_copy(&expr->type, &expr->cast.type);
		} break;

		case AST_BINARY_EXPR: {
			type_t *hint = 0;
			if (expr->binary_op.op == AST_ADD ||
				expr->binary_op.op == AST_SUB ||
				expr->binary_op.op == AST_MUL ||
				expr->binary_op.op == AST_DIV)
				hint = type_hint;

			determine_type(self, context, expr->binary_op.lhs, hint);
			determine_type(self, context, expr->binary_op.rhs, hint);
			if (expr->binary_op.lhs->type.kind == AST_NO_TYPE &&
				expr->binary_op.rhs->type.kind != AST_NO_TYPE)
				determine_type(self, context, expr->binary_op.lhs, &expr->binary_op.rhs->type);
			if (expr->binary_op.rhs->type.kind == AST_NO_TYPE &&
				expr->binary_op.lhs->type.kind != AST_NO_TYPE)
				determine_type(self, context, expr->binary_op.rhs, &expr->binary_op.lhs->type);

			if (expr->binary_op.lhs->type.kind == AST_NO_TYPE &&
				expr->binary_op.rhs->type.kind == AST_NO_TYPE)
				derror(&expr->loc, "cannot determine type of binary operation, use a cast\n");

			switch (expr->binary_op.op) {
				case AST_LT:
				case AST_GT:
				case AST_LE:
				case AST_GE:
				case AST_EQ:
				case AST_NE:
				case AST_AND:
				case AST_OR:
					bzero(&expr->type, sizeof expr->type);
					expr->type.kind = AST_BOOLEAN_TYPE;
					break;
				default:
					type_copy(&expr->type, &expr->binary_op.lhs->type);
					break;
			}
		} break;

		case AST_CONDITIONAL_EXPR: {
			type_t bool_type = { .kind = AST_BOOLEAN_TYPE };
			determine_type(self, context, expr->conditional.condition, &bool_type);

			determine_type(self, context, expr->conditional.true_expr, type_hint);
			determine_type(self, context, expr->conditional.false_expr, type_hint);

			if (!type_equal(&expr->conditional.true_expr->type, &expr->conditional.false_expr->type))
				derror(&expr->loc, "true and false expression of conditional must be of the same type");

			type_copy(&expr->type, &expr->conditional.true_expr->type);
		} break;

		case AST_ASSIGNMENT_EXPR: {
			determine_type(self, context, expr->assignment.target, type_hint);
			determine_type(self, context, expr->assignment.expr, &expr->assignment.target->type);
			if (!type_equal(&expr->assignment.target->type, &expr->assignment.expr->type)) {
				char *t1 = type_describe(&expr->assignment.target->type);
				char *t2 = type_describe(&expr->assignment.expr->type);
				derror(&expr->loc, "incompatible types in assignment ('%s' and '%s')\n", t1, t2);
				free(t1);
				free(t2);
			}
			type_copy(&expr->type, &expr->assignment.target->type);
		} break;

		case AST_COMMA_EXPR: {
			type_t *type;
			for (i = 0; i < expr->comma.num_exprs; ++i) {
				determine_type(self, context, expr->comma.exprs+i, type_hint);
				type = &expr->comma.exprs[i].type;
			}
			type_copy(&expr->type, type);
		} break;

		default:
			fprintf(stderr, "%s.%d: type determination for expr kind %d not implemented\n", __FILE__, __LINE__, expr->kind);
			abort();
			return;
	}
}


static LLVMValueRef
codegen_expr (codegen_t *self, codegen_context_t *context, expr_t *expr, char lvalue, type_t *type_hint) {
	assert(self);
	assert(context);
	assert(expr);
	/// TODO(fabianschuiki): Split this function up into three functions. One for rvalues that returns the corresponding value, one for lvalues that returns a pointer to the corresponding value, and one for rvalue pointers that returns a pointer to the corresponding value. These will be used for regular expressions, assignments, and struct member accesses respectively.

	unsigned i;
	switch (expr->kind) {

		case AST_IDENT_EXPR: {
			if (strcmp(expr->ident, "true") == 0) {
				if (lvalue)
					derror(&expr->loc, "true is not a valid lvalue\n");
				return LLVMConstNull(LLVMInt1Type());
			}

			if (strcmp(expr->ident, "false") == 0) {
				if (lvalue)
					derror(&expr->loc, "false is not a valid lvalue\n");
				return LLVMConstAllOnes(LLVMInt1Type());
			}

			codegen_symbol_t *sym = codegen_context_find_symbol(context, expr->ident);
			assert(sym && "identifier unknown");

			if (sym->kind == FUNC_SYMBOL) {
				return sym->value;
			} else if (sym->value) {
				LLVMValueRef ptr = sym->value;
				return lvalue || expr->type.kind == AST_ARRAY_TYPE ? ptr : LLVMBuildLoad(self->builder, ptr, "");
			} else {
				assert(sym->decl && sym->decl->kind == AST_CONST_DECL && "expected identifier to be a const");
				assert(!lvalue && "const is not a valid lvalue");
				LLVMValueRef value = codegen_expr(self, context, &sym->decl->cons.value, 0, type_hint);
				return value;
			}
		}

		case AST_NUMBER_LITERAL_EXPR: {
			assert(!lvalue && "number literal is not a valid lvalue");
			if (expr->type.kind == AST_NO_TYPE)
				derror(&expr->loc, "type of literal '%s' cannot be inferred from context, use a cast\n", expr->number_literal);
			if (expr->type.kind == AST_INTEGER_TYPE) {
				return LLVMConstIntOfString(codegen_type(context, &expr->type), expr->number_literal.literal, expr->number_literal.radix);
			} else if (expr->type.kind == AST_FLOAT_TYPE) {
				return LLVMConstRealOfString(codegen_type(context, &expr->type), expr->number_literal.literal);
			} else if (expr->type.kind == AST_BOOLEAN_TYPE) {
				char *p = expr->number_literal.literal;
				while (*p == '0' && *(p+1) != 0) ++p;
				if (strcmp(p, "0") == 0)
					return LLVMConstNull(LLVMInt1Type());
				else if (strcmp(p, "1") == 0)
					return LLVMConstAllOnes(LLVMInt1Type());
				else
					derror(&expr->loc, "'%s' is not a valid boolean number, can only be 0 or 1\n", expr->number_literal.literal);
			} else {
				derror(&expr->loc, "number literal can only be an integer, a boolean, or a float\n");
			}
		}

		case AST_STRING_LITERAL_EXPR: {
			assert(!lvalue && "string literal is not a valid lvalue");
			return LLVMBuildGlobalStringPtr(self->builder, expr->string_literal, ".str");
		}

		case AST_INDEX_ACCESS_EXPR: {
			LLVMValueRef target = codegen_expr(self, context, expr->index_access.target, 0, 0);
			LLVMValueRef index = codegen_expr(self, context, expr->index_access.index, 0, 0);
			if (expr->index_access.index->type.kind != AST_INTEGER_TYPE)
				derror(&expr->index_access.index->loc, "index needs to be an integer\n");
			LLVMValueRef ptr;
			if (expr->index_access.target->type.pointer > 0) {
				ptr = LLVMBuildInBoundsGEP(self->builder, target, &index, 1, "");
			} else if (expr->index_access.target->type.kind == AST_ARRAY_TYPE) {
				ptr = LLVMBuildInBoundsGEP(self->builder, target, (LLVMValueRef[]){LLVMConstNull(LLVMInt32Type()), index}, 2, "");
			} else {
				derror(&expr->loc, "cannot index into non-pointer or non-array");
			}
			return lvalue ? ptr : LLVMBuildLoad(self->builder, ptr, "");
		}

		case AST_CALL_EXPR: {
			assert(expr->call.target->kind == AST_IDENT_EXPR && "can only call functions by name at the moment");

			codegen_symbol_t *sym = codegen_context_find_symbol(context, expr->call.target->ident);
			if (!sym) {
				fprintf(stderr, "identifier '%s' unknown\n", expr->call.target->ident);
				exit(1);
			}
			if (sym->type->kind != AST_FUNC_TYPE) {
				fprintf(stderr, "identifier '%s' is not a function\n", sym->name);
				exit(1);
			}

			LLVMValueRef funcptr = sym->value;
			if (sym->kind != FUNC_SYMBOL)
				funcptr = LLVMBuildLoad(self->builder, sym->value, "funcptr");

			LLVMValueRef args[expr->call.num_args];
			for (i = 0; i < expr->call.num_args; ++i)
				args[i] = codegen_expr(self, context, &expr->call.args[i], 0, 0);
			LLVMValueRef result = LLVMBuildCall(self->builder, funcptr, args, expr->call.num_args, "");

			if (lvalue) {
				LLVMValueRef ptr = LLVMBuildAlloca(self->builder, LLVMTypeOf(result), "");
				LLVMBuildStore(self->builder, result, ptr);
				return ptr;
			} else {
				return result;
			}
		}

		case AST_MEMBER_ACCESS_EXPR: {
			type_t *st = resolve_type_name(context, &expr->member_access.target->type);
			type_t *stderef = st;
			type_t tmp;
			if (stderef->pointer > 0) {
				tmp = *stderef;
				--tmp.pointer;
				stderef = resolve_type_name(context, &tmp);
			}

			// char *sts = type_describe(stderef);
			// printf("accessing member %s of %s\n", expr->member_access.name, sts);
			// free(sts);
			if (st->pointer > 1)
				derror(&expr->loc, "cannot access member across multiple indirection\n");
			LLVMValueRef target = codegen_expr(self, context, expr->member_access.target, st->pointer == 0, 0);
			if (!target)
				derror(&expr->loc, "cannot member-access target\n");
			// LLVMValueRef struct_ptr = st->pointer == 1 ? LLVMBuildLoad(self->builder, target, "") : target;
			LLVMValueRef struct_ptr = target;
			assert(stderef->kind == AST_STRUCT_TYPE && "cannot access member of non-struct");
			for (i = 0; i < stderef->strct.num_members; ++i)
				if (strcmp(expr->member_access.name, stderef->strct.members[i].name) == 0)
					break;
			assert(i < stderef->strct.num_members && "struct has no such member");

			// char *target_ts = LLVMPrintTypeToString(LLVMTypeOf(target));
			// printf("  in LLVM accessing %s of %s\n", expr->member_access.name, target_ts);
			// LLVMDisposeMessage(target_ts);
			// type_copy(&expr->type, st->strct.members[i].type);
			LLVMValueRef ptr = LLVMBuildStructGEP(self->builder, struct_ptr, i, "");
			return lvalue ? ptr : LLVMBuildLoad(self->builder, ptr, "");
		}

		case AST_INCDEC_EXPR: {
			assert(expr->incdec_op.order == AST_PRE && "post-increment/-decrement not yet implemented");

			LLVMValueRef target = codegen_expr(self, context, expr->incdec_op.target, 1, 0);
			if (!target) {
				fprintf(stderr, "expr %d is not a valid lvalue and thus cannot be incremented/decremented\n", expr->assignment.target->kind);
				abort();
				return 0;
			}
			// expr->type = expr->incdec_op.target->type;
			if (expr->type.kind != AST_INTEGER_TYPE)
				derror(&expr->loc, "increment/decrement operator only supported for integers\n");

			LLVMValueRef value = LLVMBuildLoad(self->builder, target, "");
			LLVMValueRef const_one = LLVMConstInt(codegen_type(context, &expr->type), 1, 0);
			LLVMValueRef new_value;
			if (expr->incdec_op.direction == AST_INC) {
				new_value = LLVMBuildAdd(self->builder, value, const_one, "");
			} else {
				new_value = LLVMBuildSub(self->builder, value, const_one, "");
			}

			LLVMBuildStore(self->builder, new_value, target);
			return lvalue ? target : new_value;
		}

		case AST_UNARY_EXPR: {
			type_t *type = &expr->unary_op.target->type;
			switch (expr->unary_op.op) {
				LLVMValueRef target;

				case AST_ADDRESS:
					return codegen_expr(self, context, expr->unary_op.target, 1, 0);

				case AST_DEREF:
					target = codegen_expr(self, context, expr->unary_op.target, 0, 0);
					return lvalue ? target : LLVMBuildLoad(self->builder, target, "");

				case AST_POSITIVE:
					return codegen_expr(self, context, expr->unary_op.target, 0, 0);

				case AST_NEGATIVE:
					target = codegen_expr(self, context, expr->unary_op.target, 0, 0);
					if (type->kind == AST_INTEGER_TYPE) {
						return LLVMBuildNeg(self->builder, target, "");
					} else if (type->kind == AST_FLOAT_TYPE) {
						return LLVMBuildFNeg(self->builder, target, "");
					} else {
						char *td = type_describe(type);
						derror(&expr->loc, "expression of type %s cannot be negated\n", td);
						free(td);
					}
					break;

				case AST_BITWISE_NOT:
					if (type->kind == AST_INTEGER_TYPE) {
						target = codegen_expr(self, context, expr->unary_op.target, 0, 0);
						return LLVMBuildNot(self->builder, target, "");
					} else {
						char *td = type_describe(type);
						derror(&expr->loc, "unary operator not available for type %s\n", td);
						free(td);
					}
					break;

				case AST_NOT:
					if (type->kind == AST_BOOLEAN_TYPE) {
						target = codegen_expr(self, context, expr->unary_op.target, 0, 0);
						return LLVMBuildNot(self->builder, target, "");
					} else {
						char *td = type_describe(type);
						derror(&expr->loc, "unary operator not available for type %s\n", td);
						free(td);
					}
					break;

				default:
					fprintf(stderr, "%s.%d: codegen for unary op %d not implemented\n", __FILE__, __LINE__, expr->unary_op.op);
					abort();
			}
		}

		case AST_SIZEOF_EXPR: {
			assert(!lvalue && "result of sizeof expression is not a valid lvalue");
			if (expr->type.kind == AST_NO_TYPE)
				derror(&expr->loc, "return type of sizeof() cannot be inferred from context, use a cast\n");
			LLVMValueRef v;
			switch (expr->sizeof_op.mode) {
				case AST_EXPR_SIZEOF:
					v = LLVMSizeOf(LLVMTypeOf(codegen_expr(self, context, expr->sizeof_op.expr, 0, 0)));
					break;
				case AST_TYPE_SIZEOF:
					v = LLVMSizeOf(codegen_type(context, &expr->sizeof_op.type));
					break;
				default:
					fprintf(stderr, "%s.%d: codegen for sizeof mode %d not implemented\n", __FILE__, __LINE__, expr->sizeof_op.mode);
					abort();
			}
			return LLVMBuildIntCast(self->builder, v, codegen_type(context, &expr->type), "");
		}

		case AST_NEW_BUILTIN: {
			LLVMTypeRef type = codegen_type(context, &expr->newe.type);
			LLVMValueRef ptr = LLVMBuildMalloc(self->builder,type,"");
			LLVMBuildStore(self->builder,LLVMConstNull(type),ptr);
			return ptr;
		}

		case AST_FREE_BUILTIN: {
			LLVMValueRef ptr = codegen_expr(self, context, expr->free.expr, 0, 0);
			return LLVMBuildFree(self->builder,ptr);
		}

		case AST_CAST_EXPR: {
			assert(!lvalue && "result of a cast is not a valid lvalue");
			LLVMValueRef target = codegen_expr(self, context, expr->cast.target, 0, 0);
			if (type_equal(&expr->cast.target->type, &expr->cast.type))
				return target;
			LLVMTypeRef dst = codegen_type(context, &expr->cast.type);

			type_t *from = &expr->cast.target->type;
			type_t *to = &expr->cast.type;
			assert(from && to);
			char *from_str = type_describe(from);
			char *to_str = type_describe(to);

			if (from->pointer > 0) {
				if (to->pointer > 0) {
					return LLVMBuildPointerCast(self->builder, target, dst, "");
				} else if (to->kind == AST_INTEGER_TYPE) {
					return LLVMBuildPtrToInt(self->builder, target, dst, "");
				}
			} else if (from->kind == AST_INTEGER_TYPE) {
				if (to->pointer > 0) {
					return LLVMBuildIntToPtr(self->builder, target, dst, "");
				} else if (to->kind == AST_INTEGER_TYPE) {
					return LLVMBuildIntCast(self->builder, target, dst, "");
				} else if (to->kind == AST_BOOLEAN_TYPE) {
					return LLVMBuildICmp(self->builder, LLVMIntSGT, target, LLVMConstNull(LLVMTypeOf(target)), "");
				}
			} else if (from->kind == AST_BOOLEAN_TYPE && to->pointer == 0) {
				if (to->kind == AST_INTEGER_TYPE)
					return LLVMBuildZExt(self->builder, target, dst, "");
			} else if (from->kind == AST_ARRAY_TYPE && to->pointer > 0) {
				if (to->kind == from->array.type->kind)
					return LLVMBuildInBoundsGEP(self->builder, target, (LLVMValueRef[]){LLVMConstNull(LLVMInt32Type()), LLVMConstNull(LLVMInt32Type())}, 2, "");
			}

			derror(&expr->loc, "cannot cast from %s to %s\n", from_str, to_str);
			free(from_str);
			free(to_str);
			return 0;
		}

		case AST_BINARY_EXPR: {
			assert(!lvalue && "result of a binary operation is not a valid lvalue");
			LLVMValueRef lhs = codegen_expr(self, context, expr->binary_op.lhs, 0, 0);
			LLVMValueRef rhs = codegen_expr(self, context, expr->binary_op.rhs, 0, 0);
			if (!type_equal(&expr->binary_op.lhs->type, &expr->binary_op.rhs->type))
				derror(&expr->loc, "operands to binary operator must be of the same type");
			// expr->type = expr->assignment.expr->type;

			type_t *t = &expr->binary_op.lhs->type;
			if (t->kind == AST_BOOLEAN_TYPE) {
				switch (expr->binary_op.op) {
					case AST_EQ:
						return LLVMBuildICmp(self->builder, LLVMIntEQ, lhs, rhs, "");
					case AST_NE:
						return LLVMBuildICmp(self->builder, LLVMIntNE, lhs, rhs, "");
					case AST_AND:
						return LLVMBuildAnd(self->builder, lhs, rhs, "");
					case AST_OR:
						return LLVMBuildOr(self->builder, lhs, rhs, "");
					default:
						fprintf(stderr, "%s.%d: codegen for binary op %d on boolean types not implemented\n", __FILE__, __LINE__, expr->binary_op.op);
						abort();
				}
			} else if (t->kind == AST_INTEGER_TYPE) {
				switch (expr->binary_op.op) {
					case AST_ADD:
						return LLVMBuildAdd(self->builder, lhs, rhs, "");
					case AST_SUB:
						return LLVMBuildSub(self->builder, lhs, rhs, "");
					case AST_MUL:
						return LLVMBuildMul(self->builder, lhs, rhs, "");
					case AST_DIV:
						return LLVMBuildSDiv(self->builder, lhs, rhs, "");
					case AST_MOD:
						return LLVMBuildSRem(self->builder, lhs, rhs, "");
					case AST_LEFT:
						return LLVMBuildShl(self->builder, lhs, rhs, "");
					case AST_RIGHT:
						return LLVMBuildAShr(self->builder, lhs, rhs, "");
					case AST_LT:
						return LLVMBuildICmp(self->builder, LLVMIntSLT, lhs, rhs, "");
					case AST_GT:
						return LLVMBuildICmp(self->builder, LLVMIntSGT, lhs, rhs, "");
					case AST_LE:
						return LLVMBuildICmp(self->builder, LLVMIntSLE, lhs, rhs, "");
					case AST_GE:
						return LLVMBuildICmp(self->builder, LLVMIntSGE, lhs, rhs, "");
					case AST_EQ:
						return LLVMBuildICmp(self->builder, LLVMIntEQ, lhs, rhs, "");
					case AST_NE:
						return LLVMBuildICmp(self->builder, LLVMIntNE, lhs, rhs, "");
					case AST_BITWISE_AND:
						return LLVMBuildAnd(self->builder, lhs, rhs, "");
					case AST_BITWISE_XOR:
						return LLVMBuildXor(self->builder, lhs, rhs, "");
					case AST_BITWISE_OR:
						return LLVMBuildOr(self->builder, lhs, rhs, "");
					case AST_AND:
					case AST_OR:
						derror(&expr->loc, "binary operator not available for integer types\n");
					default:
						fprintf(stderr, "%s.%d: codegen for binary op %d on integer types not implemented\n", __FILE__, __LINE__, expr->binary_op.op);
						abort();
				}
			} else if (t->kind == AST_FLOAT_TYPE) {
				switch (expr->binary_op.op) {
					case AST_ADD:
						return LLVMBuildFAdd(self->builder, lhs, rhs, "");
					case AST_SUB:
						return LLVMBuildFSub(self->builder, lhs, rhs, "");
					case AST_MUL:
						return LLVMBuildFMul(self->builder, lhs, rhs, "");
					case AST_DIV:
						return LLVMBuildFDiv(self->builder, lhs, rhs, "");
					case AST_MOD:
						return LLVMBuildFRem(self->builder, lhs, rhs, "");
					case AST_LEFT:
					case AST_RIGHT:
					case AST_LT:
						return LLVMBuildFCmp(self->builder, LLVMRealOLT, lhs, rhs, "");
					case AST_GT:
						return LLVMBuildFCmp(self->builder, LLVMRealOGT, lhs, rhs, "");
					case AST_LE:
						return LLVMBuildFCmp(self->builder, LLVMRealOLE, lhs, rhs, "");
					case AST_GE:
						return LLVMBuildFCmp(self->builder, LLVMRealOGE, lhs, rhs, "");
					case AST_EQ:
						return LLVMBuildFCmp(self->builder, LLVMRealOEQ, lhs, rhs, "");
					case AST_NE:
						return LLVMBuildFCmp(self->builder, LLVMRealONE, lhs, rhs, "");
					case AST_BITWISE_AND:
					case AST_BITWISE_XOR:
					case AST_BITWISE_OR:
					case AST_AND:
					case AST_OR:
						derror(&expr->loc, "binary operator not available for floating point types\n");
					default:
						fprintf(stderr, "%s.%d: codegen for binary op %d on floating point types not implemented\n", __FILE__, __LINE__, expr->binary_op.op);
						abort();
				}
			} else {
				char *td = type_describe(t);
				derror(&expr->loc, "invalid type %s to binary operator\n", td);
				free(td);
			}
		}

		case AST_CONDITIONAL_EXPR: {
			assert(!lvalue && "result of a conditional is not a valid lvalue");
			LLVMValueRef cond = codegen_expr(self, context, expr->conditional.condition, 0, 0);

			LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(self->builder));
			LLVMBasicBlockRef true_block = LLVMAppendBasicBlock(func, "iftrue");
			LLVMBasicBlockRef false_block = LLVMAppendBasicBlock(func, "iffalse");
			LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "ifexit");
			LLVMBuildCondBr(self->builder, cond, true_block, false_block ? false_block : exit_block);

			LLVMPositionBuilderAtEnd(self->builder, true_block);
			LLVMValueRef true_value = codegen_expr(self, context, expr->conditional.true_expr, 0, 0);
			LLVMBuildBr(self->builder, exit_block);

			LLVMPositionBuilderAtEnd(self->builder, false_block);
			LLVMValueRef false_value = codegen_expr(self, context, expr->conditional.false_expr, 0, 0);
			LLVMBuildBr(self->builder, exit_block);

			// TODO: Make sure types of true and false branch match, otherwise cast.
			expr->type = expr->conditional.true_expr->type;

			LLVMPositionBuilderAtEnd(self->builder, exit_block);
			LLVMValueRef incoming_values[2] = { true_value, false_value };
			LLVMBasicBlockRef incoming_blocks[2] = { true_block, false_block };
			LLVMValueRef phi = LLVMBuildPhi(self->builder, codegen_type(context, &expr->conditional.true_expr->type), "");
			LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);
			return phi;
		}

		case AST_ASSIGNMENT_EXPR: {
			LLVMValueRef lv = codegen_expr(self, context, expr->assignment.target, 1, 0);
			if (!lv) {
				fprintf(stderr, "expr %d cannot be assigned a value\n", expr->assignment.target->kind);
				abort();
				return 0;
			}
			LLVMValueRef rv = codegen_expr(self, context, expr->assignment.expr, 0, 0);
			// assert(expr->assignment.target->type.kind == expr->assignment.expr->type.kind &&
			//        expr->assignment.target->type.width == expr->assignment.expr->type.width &&
			//        expr->assignment.target->type.pointer == expr->assignment.expr->type.pointer &&
			//        "incompatible types in assignment");
			// expr->type = expr->assignment.expr->type;
			LLVMValueRef v;
			if (expr->assignment.op == AST_ASSIGN) {
				v = rv;
			} else {
				LLVMValueRef dv = LLVMBuildLoad(self->builder, lv, "");
				switch (expr->assignment.op) {
					case AST_ADD_ASSIGN: v = LLVMBuildAdd(self->builder, dv, rv, ""); break;
					case AST_SUB_ASSIGN: v = LLVMBuildSub(self->builder, dv, rv, ""); break;
					case AST_MUL_ASSIGN: v = LLVMBuildMul(self->builder, dv, rv, ""); break;
					case AST_DIV_ASSIGN: v = LLVMBuildUDiv(self->builder, dv, rv, ""); break;
					default:
						fprintf(stderr, "%s.%d: codegen for assignment op %d not implemented\n", __FILE__, __LINE__, expr->assignment.op);
						abort();
						return 0;
				}
			}
			LLVMBuildStore(self->builder, v, lv);
			return rv;
		}

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
			fprintf(stderr, "%s.%d: codegen for expr kind %d not implemented\n", __FILE__, __LINE__, expr->kind);
			abort();
			return 0;
	}
}


static LLVMValueRef
codegen_expr_top (codegen_t *self, codegen_context_t *context, expr_t *expr, char lvalue, type_t *type_hint) {
	determine_type(self, context, expr, type_hint);
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
				LLVMValueRef var = LLVMBuildAlloca(self->builder, codegen_type(context, &decl->variable.type), decl->variable.name);

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
				}
			} else {
				LLVMValueRef val = codegen_expr_top(self, context, decl->variable.initial, 0, 0);
				if (decl->variable.initial->type.kind == AST_NO_TYPE)
					derror(&decl->loc, "type of variable '%s' could not be inferred from its initial value\n", decl->variable.name);
				type_copy(&decl->variable.type, &decl->variable.initial->type);

				char *td = type_describe(&decl->variable.type);
				printf("deduced variable %s type to be %s\n", decl->variable.name, td);
				free(td);

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
				sym.value = global;
			}

			codegen_context_add_symbol(context, &sym);
			break;
		}

		default:
			fprintf(stderr, "%s.%d: codegen for decl kind %d not implemented\n", __FILE__, __LINE__, decl->kind);
			abort();
			break;
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
			LLVMValueRef cond = codegen_expr_top(self, &subctx, stmt->iteration.condition, 0, &bool_type);
			LLVMBuildCondBr(self->builder, cond, body_block, exit_block);

			// Execute the loop body.
			LLVMPositionBuilderAtEnd(self->builder, body_block);
			if (stmt->iteration.stmt)
				codegen_stmt(&cg, &subctx, stmt->iteration.stmt);
			LLVMBuildBr(self->builder, step_block);

			// Execute the loop step.
			LLVMPositionBuilderAtEnd(self->builder, step_block);
			if (stmt->iteration.step)
				codegen_expr_top(self, &subctx, stmt->iteration.step, 0, 0);
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
