/* Copyright (c) 2015 Fabian Schuiki */
#include "codegen_internal.h"


PREPARE_TYPE(member_access_expr) {
	unsigned i;
	prepare_expr(self, context, expr->member_access.target, 0);
	type_t *st = resolve_type_name(context, &expr->member_access.target->type);
	if (st->kind == AST_INTERFACE_TYPE) {
		if (st->pointer > 0)
			derror(&expr->loc, "cannot access member of pointer to an interface, dereference the pointer\n");
		for (i = 0; i < st->interface.num_members; ++i) {
			interface_member_t *m = st->interface.members+i;
			if (m->kind == AST_MEMBER_FIELD && strcmp(expr->member_access.name, m->field.name) == 0)
				break;
		}
		if (i == st->interface.num_members)
			derror(&expr->loc, "interface has no member named '%s'\n", expr->member_access.name);
		type_copy(&expr->type, st->interface.members[i].field.type);
	} else {
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
	}
}


CODEGEN_EXPR(member_access_expr) {
	unsigned i;
	type_t *st = resolve_type_name(context, &expr->member_access.target->type);
	type_t *stderef = st;
	type_t tmp;
	if (stderef->pointer > 0) {
		tmp = *stderef;
		--tmp.pointer;
		stderef = resolve_type_name(context, &tmp);
	}

	LLVMValueRef ptr;
	if (st->kind == AST_INTERFACE_TYPE) {
		for (i = 0; i < st->interface.num_members; ++i) {
			interface_member_t *m = st->interface.members+i;
			if (m->kind == AST_MEMBER_FIELD && strcmp(expr->member_access.name, m->field.name) == 0)
				break;
		}
		assert(i < st->interface.num_members && "interface has no such member");
		LLVMValueRef target = codegen_expr(self, context, expr->member_access.target, 0, 0);
		assert(target);
		LLVMValueRef table_ptr = LLVMBuildExtractValue(self->builder, target, 0, "table_ptr");
		LLVMValueRef object_ptr = LLVMBuildExtractValue(self->builder, target, 1, "object_ptr");

		LLVMValueRef table = LLVMBuildLoad(self->builder, table_ptr, "table");
		LLVMValueRef member_offset = LLVMBuildExtractValue(self->builder, table, 1+i, "member_offset");
		LLVMValueRef member_offset_int = LLVMBuildPtrToInt(self->builder, member_offset, LLVMInt64Type(), "member_offset_int");
		LLVMValueRef ptr_bytes = LLVMBuildInBoundsGEP(self->builder, object_ptr, &member_offset_int, 1, "ptr_bytes");
		ptr = LLVMBuildPointerCast(self->builder, ptr_bytes, LLVMTypeOf(member_offset), "");
	} else {
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
		ptr = LLVMBuildStructGEP(self->builder, struct_ptr, i, "");
	}

	return lvalue ? ptr : LLVMBuildLoad(self->builder, ptr, "");
}
