/* Copyright (c) 2015-2016 Fabian Schuiki */
#include "codegen_internal.h"


PREPARE_EXPR(assignment_expr) {
	prepare_expr(self, context, expr->assignment.target, type_hint);
	prepare_expr(self, context, expr->assignment.expr, &expr->assignment.target->type);
	if (!type_equal(&expr->assignment.target->type, &expr->assignment.expr->type)) {
		char *t1 = type_describe(&expr->assignment.target->type);
		char *t2 = type_describe(&expr->assignment.expr->type);
		derror(&expr->loc, "incompatible types in assignment ('%s' and '%s')\n", t1, t2);
		free(t1);
		free(t2);
	}
	type_copy(&expr->type, &expr->assignment.target->type);
}


CODEGEN_EXPR(assignment_expr) {
	LLVMValueRef lv = codegen_expr(self, context, expr->assignment.target, 1, 0);
	if (!lv) {
		fprintf(stderr, "expr %d cannot be assigned a value\n", expr->assignment.target->kind);
		abort();
		return 0;
	}
	LLVMValueRef rv = codegen_expr(self, context, expr->assignment.expr, 0, 0);
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
