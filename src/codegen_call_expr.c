/* Copyright (c) 2015-2016 Fabian Schuiki */
#include "codegen_internal.h"

// TODO(fabianschuiki): Check whether the arguments passed to the function call
// are compatible with the function prototype. Passing an incorrect number of
// arguments currently only fails in LLVM with an ugly assertion. This should be
// a proper lowc error.

PREPARE_EXPR(call_expr) {
	unsigned i;
	expr_t *tgt = expr->call.target;
	if (tgt->kind == AST_IDENT_EXPR) {
		codegen_symbol_t *sym = codegen_context_find_symbol(context, tgt->ident);
		if (!sym)
			derror(&expr->loc, "identifier '%s' unknown\n", tgt->ident);
		if (sym->kind == INTERFACE_FUNCTION_SYMBOL) {
			interface_member_t *m = sym->interface->interface.members+sym->member;
			assert(m->kind == AST_MEMBER_FUNCTION);
			type_copy(&expr->type, m->func.return_type);
			for (i = 0; i < expr->call.num_args; ++i)
				prepare_expr(self, context, expr->call.args+i, m->func.args+i);
		} else {
			if (!sym->type || sym->type->kind != AST_FUNC_TYPE)
				derror(&expr->loc, "identifier '%s' is not a function\n", sym->name);
			type_copy(&expr->type, sym->type->func.return_type);
			for (i = 0; i < expr->call.num_args; ++i)
				prepare_expr(self, context, expr->call.args+i, sym->type->func.args+i);
		}
	} else if (tgt->kind == AST_MEMBER_ACCESS_EXPR) {
		prepare_expr(self, context, tgt->member_access.target, 0);
		type_t *intf = resolve_type_name(context, &tgt->member_access.target->type);
		if (intf->kind == AST_INTERFACE_TYPE) {
			derror(&tgt->loc, "interface calls not yet implemented\n");
		} else {
			derror(&tgt->loc, "non-interface cannot be called\n");
		}
	} else {
		derror(&expr->loc, "expression cannot be called\n");
	}
}


CODEGEN_EXPR(call_expr) {
	assert(expr->call.target->kind == AST_IDENT_EXPR && "can only call functions by name at the moment");

	codegen_symbol_t *sym = codegen_context_find_symbol(context, expr->call.target->ident);
	if (!sym) {
		fprintf(stderr, "identifier '%s' unknown\n", expr->call.target->ident);
		exit(1);
	}

	unsigned i;
	LLVMValueRef result;
	if (sym->kind == INTERFACE_FUNCTION_SYMBOL) {
		interface_member_t *m = sym->interface->interface.members+sym->member;
		if (m->func.num_args != expr->call.num_args)
			derror(&expr->loc, "'%s' requires %d arguments, but only %d given\n", sym->name, m->func.num_args, expr->call.num_args);

		// Find the placeholder argument in the function.
		unsigned ph_idx;
		for (ph_idx = 0; ph_idx < m->func.num_args; ++ph_idx)
			if (m->func.args[ph_idx].kind == AST_PLACEHOLDER_TYPE)
				break;
		if (ph_idx == m->func.num_args)
			derror(&expr->loc, "unable to find a placeholder argument\n");

		// Verify that the argument supplied for the placeholder is an
		// interface of the required type.
		expr_t *ph = expr->call.args+ph_idx;
		type_t *ph_type = resolve_type_name(context, &ph->type);
		if (ph_type->kind != AST_INTERFACE_TYPE)
			derror(&ph->loc, "argument %d of '%s' should be an interface\n", ph_idx, sym->name);
		if (!type_equal(ph_type, sym->interface))
			derror(&ph->loc, "argument %d of '%s' is an incompatible interface\n", ph_idx, sym->name);


		// Load the interface table of the placeholder.
		LLVMValueRef target = codegen_expr(self, context, ph, 0, 0);
		assert(target);
		LLVMValueRef table_ptr = LLVMBuildExtractValue(self->builder, target, 0, "table_ptr");
		LLVMValueRef object_ptr = LLVMBuildExtractValue(self->builder, target, 1, "object_ptr");

		LLVMValueRef table = LLVMBuildLoad(self->builder, table_ptr, "table");
		LLVMValueRef member_func = LLVMBuildExtractValue(self->builder, table, 1+sym->member, m->func.name);

		// Generate the code for the arguments.
		LLVMValueRef args[expr->call.num_args];
		for (i = 0; i < expr->call.num_args; ++i) {
			if (i == ph_idx)
				args[i] = object_ptr;
			else
				args[i] = codegen_expr(self, context, &expr->call.args[i], 0, 0);
		}
		result = LLVMBuildCall(self->builder, member_func, args, expr->call.num_args, "");

	} else {
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
		result = LLVMBuildCall(self->builder, funcptr, args, expr->call.num_args, "");
	}

	if (lvalue) {
		LLVMValueRef ptr = LLVMBuildAlloca(self->builder, LLVMTypeOf(result), "");
		LLVMBuildStore(self->builder, result, ptr);
		return ptr;
	} else {
		return result;
	}
}
