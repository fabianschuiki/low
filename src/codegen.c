/* Copyright (c) 2015 Fabian Schuiki, Thomas Richner */
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


static LLVMTypeRef
make_interface_table_type (context_t *context, interface_member_t *members, unsigned num_members) {
	assert(context);
	assert(members || num_members == 0);
	unsigned i,n;

	unsigned num_fields = 1+num_members;
	LLVMTypeRef fields[num_fields];
	fields[0] = LLVMPointerType(LLVMInt8Type(), 0); // pointer to the typeinfo, unused for now
	for (i = 0; i < num_members; ++i) {
		switch (members[i].kind) {
			case AST_MEMBER_FIELD:
				fields[1+i] = LLVMPointerType(codegen_type(context, members[i].field.type), 0);
				break;
			case AST_MEMBER_FUNCTION: {
				LLVMTypeRef args[members[i].func.num_args];
				for (n = 0; n < members[i].func.num_args; ++n) {
					if (members[i].func.args[n].kind == AST_PLACEHOLDER_TYPE)
						args[n] = LLVMPointerType(LLVMInt8Type(), 0);
					else
						args[n] = codegen_type(context, members[i].func.args+n);
				}
				LLVMTypeRef ft = LLVMFunctionType(codegen_type(context, members[i].func.return_type), args, members[i].func.num_args, 0);
				fields[1+i] = LLVMPointerType(ft, 0);
			} break;
			default:
				die("codegen for interface member kind %d not implemented", members[i].kind);
		}
	}

	return LLVMStructType(fields, num_fields, 0);
}


LLVMTypeRef
codegen_type (context_t *context, type_t *type) {
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
		case AST_SLICE_TYPE: {
			// underlying struct of a slice
			LLVMTypeRef members[3];
			members[0] = LLVMPointerType(LLVMArrayType(codegen_type(context, type->slice.type),0),0); // pointer to array
			members[1] = LLVMIntType(64); 			// length @HARDCODED
			members[2] = LLVMIntType(64); 			// capacity @HARDCODED
			return LLVMStructType(members, 3, 0); 	// NOT PACKED
		}
		case AST_ARRAY_TYPE: {
			LLVMTypeRef element = codegen_type(context, type->array.type);
			return LLVMArrayType(element, type->array.length);
		}
		case AST_INTERFACE_TYPE: {
			LLVMTypeRef fields[] = {
				LLVMPointerType(make_interface_table_type(context, type->interface.members, type->interface.num_members), 0),
				LLVMPointerType(LLVMInt8Type(), 0), // pointer to the object
			};
			for (i = 0; i < type->interface.num_members; ++i) {
				codegen_symbol_t sym = {
					.kind = INTERFACE_FUNCTION_SYMBOL,
					.name = type->interface.members[i].func.name,
					.interface = type,
					.member = i,
				};
				codegen_context_add_symbol(context, &sym);
			}
			return LLVMStructType(fields, 2, 0);
		}
		default:
			fprintf(stderr, "%s.%d: codegen for type kind %d not implemented\n", __FILE__, __LINE__, type->kind);
			abort();
	}
}


void
determine_type (codegen_t *self, codegen_context_t *context, expr_t *expr, type_t *type_hint) {
	assert(self);
	assert(context);
	assert(expr);

	unsigned i;
	switch (expr->kind) {

		case AST_IDENT_EXPR: return determine_type_ident_expr(self, context, expr, type_hint);

		case AST_NUMBER_LITERAL_EXPR: return determine_type_number_literal_expr(self, context, expr, type_hint);

		case AST_STRING_LITERAL_EXPR: return determine_type_string_literal_expr(self, context, expr, type_hint);

		case AST_INDEX_ACCESS_EXPR: return determine_type_index_access_expr(self, context, expr, type_hint);

		case AST_CALL_EXPR: return determine_type_call_expr(self, context, expr, type_hint);

		case AST_MEMBER_ACCESS_EXPR: return determine_type_member_access_expr(self, context, expr, type_hint);

		case AST_INCDEC_EXPR: {
			determine_type(self, context, expr->incdec_op.target, type_hint);
			type_copy(&expr->type, &expr->incdec_op.target->type);
		} break;

		case AST_UNARY_EXPR: return determine_type_unary_expr(self, context, expr, type_hint);

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

		case AST_MAKE_BUILTIN: {
			if (expr->make.type.kind != AST_SLICE_TYPE) {
				derror(&expr->loc, "cannot make non-slice\n");
			}
			type_t int_type = {.kind=AST_INTEGER_TYPE,.width=64};
			determine_type(self,context,expr->make.expr, &int_type);
			type_copy(&expr->type, &expr->make.type);
		} break;

		case AST_LENCAP_BUILTIN: {
			determine_type(self,context,expr->lencap.expr, 0);

			if (expr->lencap.expr->type.kind != AST_SLICE_TYPE) {
				derror(&expr->loc, "cannot use with non-slice type\n");
			}

			type_t int_type = {.kind=AST_INTEGER_TYPE,.width=64};
			type_copy(&expr->type, &int_type);
		} break;

		case AST_CAST_EXPR: return determine_type_cast_expr(self, context, expr, type_hint);

		case AST_BINARY_EXPR: return determine_type_binary_expr(self, context, expr, type_hint);

		case AST_CONDITIONAL_EXPR: return determine_type_conditional_expr(self, context, expr, type_hint);

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

static void
dump_val(char* name,LLVMValueRef val){
	printf("-- %s --:\n",name);
	printf("Value: \t"); fflush(stdout); LLVMDumpValue(val);
	printf("Type: \t"); fflush(stdout); LLVMDumpType(LLVMTypeOf(val));
	printf("--    --\n"); fflush(stdout);
}

static void
dump_type(char* name,LLVMTypeRef t){
	printf("-- %s --:\n",name);
	printf("Type: \t"); fflush(stdout); LLVMDumpType(t);
	printf("--    --\n"); fflush(stdout);
}

/*
 * generates IR to allocate zero initialised array on heap, return pointer to new array
 * similar to `calloc(type,size)`
 */
static LLVMValueRef
codegen_array_new(codegen_t* self, codegen_context_t *context,LLVMTypeRef type,LLVMValueRef size){
	//---- malloc on heap
	LLVMValueRef arrptr = LLVMBuildArrayMalloc(self->builder,type,size,"");

	//TODO check for nonzero

	//---- zero initialise
	LLVMValueRef cntrptr = LLVMBuildAlloca(self->builder,LLVMTypeOf(size),"cntrptr");
	LLVMBuildStore(self->builder,size,cntrptr);

	LLVMValueRef func = LLVMGetBasicBlockParent(LLVMGetInsertBlock(self->builder));
	LLVMBasicBlockRef loop_block = LLVMAppendBasicBlock(func, "loopcond");
	LLVMBasicBlockRef body_block = LLVMAppendBasicBlock(func, "loopbody");
	LLVMBasicBlockRef exit_block = LLVMAppendBasicBlock(func, "loopexit");
	LLVMBuildBr(self->builder, loop_block);

	// Check the loop condition.
	LLVMPositionBuilderAtEnd(self->builder, loop_block);
	LLVMValueRef cntr = LLVMBuildLoad(self->builder,cntrptr,"cntr");
	LLVMValueRef cond = LLVMBuildICmp(self->builder,LLVMIntSLE,LLVMConstNull(LLVMTypeOf(cntr)),cntr,"cond");
	LLVMBuildCondBr(self->builder, cond, body_block, exit_block);

	// Execute the loop body.
	LLVMPositionBuilderAtEnd(self->builder, body_block);
	//LLVMValueRef iptr  = LLVMBuildInBoundsGEP(self->builder, arrptr, (LLVMValueRef[]){LLVMConstNull(LLVMInt32Type()), cntrptr}, 2, "");
	LLVMValueRef iptr  = LLVMBuildInBoundsGEP(self->builder, arrptr, (LLVMValueRef[]){cntr}, 1, "");
	LLVMBuildStore(self->builder,LLVMConstNull(type),iptr);
	cntr = LLVMBuildSub(self->builder,cntr,LLVMConstInt(LLVMTypeOf(cntr),1,0),"");
	LLVMBuildStore(self->builder,cntr,cntrptr);
	LLVMBuildBr(self->builder, loop_block);

	LLVMPositionBuilderAtEnd(self->builder, exit_block);

	return arrptr;
}

/* struct {
 *    *arr: *[i32 x 0]
 *    len: i64
 *    cap: i64
 * }
 */

static LLVMValueRef
codegen_slice_new(codegen_t *self, codegen_context_t *context, make_builtin_t *make){
	assert(self);
	assert(context);
	assert(make);

	//---- init cap to given value
	LLVMValueRef caparg = codegen_expr(self, context, make->expr, 0, 0);
	caparg = LLVMBuildIntCast(self->builder, caparg, LLVMInt64Type(),"");

	//---- alloc array on heap
	LLVMTypeRef element_type = codegen_type(context, make->type.slice.type); // array element type
	LLVMValueRef arrptr = codegen_array_new(self,context,element_type,caparg);
	LLVMTypeRef array_type = LLVMPointerType(LLVMArrayType(element_type,0),0);
	arrptr = LLVMBuildPointerCast(self->builder,arrptr,array_type,"");

	//---- assemble struct
	LLVMTypeRef make_type = codegen_type(context, &make->type);
	LLVMValueRef slice = LLVMConstNull(make_type);

	slice = LLVMBuildInsertValue(self->builder, slice, arrptr, 0, "arrptr");
	slice = LLVMBuildInsertValue(self->builder, slice, caparg, 2, "cap");

	return slice;
}

LLVMValueRef
codegen_expr (codegen_t *self, codegen_context_t *context, expr_t *expr, char lvalue, type_t *type_hint) {
	assert(self);
	assert(context);
	assert(expr);
	/// TODO(fabianschuiki): Split this function up into three functions. One for rvalues that returns the corresponding value, one for lvalues that returns a pointer to the corresponding value, and one for rvalue pointers that returns a pointer to the corresponding value. These will be used for regular expressions, assignments, and struct member accesses respectively.

	unsigned i;
	switch (expr->kind) {

		case AST_IDENT_EXPR: return codegen_ident_expr(self, context, expr, lvalue);

		case AST_NUMBER_LITERAL_EXPR: return codegen_number_literal_expr(self, context, expr, lvalue);

		case AST_STRING_LITERAL_EXPR: return codegen_string_literal_expr(self, context, expr, lvalue);

		case AST_INDEX_ACCESS_EXPR: return codegen_index_access_expr(self, context, expr, lvalue);

		case AST_CALL_EXPR: return codegen_call_expr(self, context, expr, lvalue);

		case AST_MEMBER_ACCESS_EXPR: return codegen_member_access_expr(self, context, expr, lvalue);

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

		case AST_UNARY_EXPR: return codegen_unary_expr(self, context, expr, lvalue);

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

		case AST_MAKE_BUILTIN: {
			assert(expr->make.type.kind==AST_SLICE_TYPE && "MAKE only for slices defined");
			return codegen_slice_new(self,context,&expr->make);
		}

		case AST_LENCAP_BUILTIN: {
			assert(expr->lencap.expr->type.kind==AST_SLICE_TYPE && "CAP/LEN only for slices defined");
			LLVMValueRef aslice = codegen_expr(self, context, expr->lencap.expr, 0, 0);

			//LLVMTypeRef type = codegen_type(context, &expr->lencap.expr->type);

			LLVMValueRef eptr = NULL;
			if(expr->lencap.kind==AST_LEN){
				eptr = LLVMBuildStructGEP(self->builder,aslice,1,"len");
			}else if(expr->lencap.kind==AST_CAP){
				eptr = LLVMBuildStructGEP(self->builder,aslice,2,"cap");
			}else{
				derror(&expr->loc,"what a terrible failure :(");
			}

			return LLVMBuildLoad(self->builder,eptr,"");
		}

		case AST_CAST_EXPR: return codegen_cast_expr(self, context, expr, lvalue);

		case AST_BINARY_EXPR: return codegen_binary_expr(self, context, expr, lvalue);

		case AST_CONDITIONAL_EXPR: return codegen_conditional_expr(self, context, expr, lvalue);

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
