/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"


PREPARE_TYPE(ident_expr) {
	if (strcmp(expr->ident, "true") == 0 || strcmp(expr->ident, "false") == 0) {
		expr->type.kind = AST_BOOLEAN_TYPE;
		return;
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
		prepare_expr(self, context, &sym->decl->cons.value, type_hint);
		type_copy(&expr->type, &sym->decl->cons.value.type);
	}
}


CODEGEN_EXPR(ident_expr) {
	if (strcmp(expr->ident, "true") == 0) {
		if (lvalue)
			derror(&expr->loc, "true is not a valid lvalue\n");
		return LLVMConstAllOnes(LLVMInt1Type());
	}

	if (strcmp(expr->ident, "false") == 0) {
		if (lvalue)
			derror(&expr->loc, "false is not a valid lvalue\n");
		return LLVMConstNull(LLVMInt1Type());
	}

	codegen_symbol_t *sym = codegen_context_find_symbol(context, expr->ident);
	assert(sym && "identifier unknown");

	if (sym->kind == FUNC_SYMBOL) {
		return sym->value;
	} else if (sym->value) {
		LLVMValueRef ptr = sym->value;
		return lvalue || expr->type.kind == AST_SLICE_TYPE ? ptr : LLVMBuildLoad(self->builder, ptr, "");
	} else {
		assert(sym->decl && sym->decl->kind == AST_CONST_DECL && "expected identifier to be a const");
		assert(!lvalue && "const is not a valid lvalue");
		LLVMValueRef value = codegen_expr(self, context, &sym->decl->cons.value, 0, 0);
		return value;
	}
}
