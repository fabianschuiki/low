/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"


PREPARE_TYPE(cast_expr) {
	prepare_expr(self, context, expr->cast.target, &expr->cast.type);
	type_copy(&expr->type, &expr->cast.type);
}


CODEGEN_EXPR(cast_expr) {
	unsigned i,n;
	assert(!lvalue && "result of a cast is not a valid lvalue");
	LLVMValueRef target = codegen_expr(self, context, expr->cast.target, 0, 0);
	if (type_equal(&expr->cast.target->type, &expr->cast.type))
		return target;
	LLVMTypeRef dst = codegen_type(context, &expr->cast.type);

	type_t *from = resolve_type_name(context, &expr->cast.target->type);
	type_t *to = resolve_type_name(context, &expr->cast.type);
	assert(from && to);
	char *from_str = type_describe(from);
	char *to_str = type_describe(to);
	from = resolve_type_name(context, from);
	to = resolve_type_name(context, to);

	if (to->kind == AST_INTERFACE_TYPE) {
		if (from->pointer == 1) {
			type_t refd;
			type_copy(&refd, from);
			--refd.pointer;
			type_t *resolved = resolve_type_name(context, &refd);

			if (resolved->kind == AST_STRUCT_TYPE) {
				// LLVMTypeRef table_type = make_interface_table_type(context, to->interface.members, to->interface.num_members);
				unsigned num_fields = 1+to->interface.num_members;
				LLVMValueRef fields[num_fields];
				fields[0] = LLVMConstNull(LLVMPointerType(LLVMInt8Type(), 0));
				for (i = 0; i < to->interface.num_members; ++i) {
					interface_member_t *m = to->interface.members+i;
					switch (m->kind) {
						case AST_MEMBER_FUNCTION: {

							// Try to lookup which function implements this interface member.
							const char *mapped = codegen_context_find_mapping(context, &expr->cast.type, &refd, m->func.name);
							if (!mapped)
								derror(&expr->loc, "cannot determine which function maps to '%s'\n", m->func.name);

							// Find the implementing function.
							codegen_symbol_t *sym = codegen_context_find_symbol(context, mapped);
							if (!sym)
								derror(&expr->loc, "function '%s' which is supposed to implement '%s' is unknown\n", mapped, m->func.name);

							// Store a pointer to the function.
							// fields[1+i] = sym->value;
							LLVMTypeRef args[m->func.num_args];
							for (n = 0; n < m->func.num_args; ++n) {
								if (m->func.args[n].kind == AST_PLACEHOLDER_TYPE)
									args[n] = LLVMPointerType(LLVMInt8Type(), 0);
								else
									args[n] = codegen_type(context, m->func.args+n);
							}
							LLVMTypeRef ft = LLVMPointerType(LLVMFunctionType(codegen_type(context, m->func.return_type), args, m->func.num_args, 0), 0);
							fields[1+i] = LLVMBuildPointerCast(self->builder, sym->value, ft, "");
							// fields[1+i] = LLVMConstNull(ft);
						} break;

						case AST_MEMBER_FIELD: {
							unsigned n;
							for (n = 0; n < resolved->strct.num_members; ++n)
								if (strcmp(resolved->strct.members[n].name, m->field.name) == 0)
									break;
							if (n == resolved->strct.num_members)
								derror(&expr->loc, "type %s has no member named '%s' required by the interface %s", from_str, m->field.name, to_str);
							if (!type_equal(m->field.type, resolved->strct.members[n].type))
								derror(&expr->loc, "field '%s' is of the wrong type for interface %s", m->field.name, to_str);
							fields[1+i] = LLVMBuildStructGEP(self->builder, LLVMConstNull(LLVMTypeOf(target)), n, "member");
						} break;

						default:
							die("mapping of interface member kind %d not implemented", m->kind);
					}
				}
				LLVMValueRef table = LLVMConstStruct(fields, num_fields, 0);
				LLVMValueRef table_global = LLVMAddGlobal(self->module, LLVMTypeOf(table), "interface_table");
				LLVMSetInitializer(table_global, table);
				LLVMValueRef target_cast = LLVMBuildPointerCast(self->builder, target, LLVMPointerType(LLVMInt8Type(), 0), "");

				LLVMValueRef result = LLVMConstNull(dst);
				result = LLVMBuildInsertValue(self->builder, result, table_global, 0, "");
				result = LLVMBuildInsertValue(self->builder, result, target_cast, 1, "");
				return result;
			} else {
				derror(&expr->loc, "type %s is not a pointer to a struct and can therefore not be cast to an interface\n", from_str);
			}
		} else {
			derror(&expr->loc, "only pointers can be cast to an interface\n");
		}
	} else if (from->pointer > 0) {
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
