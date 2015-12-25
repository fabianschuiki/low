/* Copyright (c) 2015 Fabian Schuiki, Thomas Richner */
#include "ast.h"
#include "common.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void
expr_dispose (expr_t *self) {
	if (self == 0)
		return;

	unsigned i;
	switch (self->kind) {
		case AST_IDENT_EXPR:
			free(self->ident);
			break;
		case AST_STRING_LITERAL_EXPR:
			free(self->string_literal);
			break;
		case AST_NUMBER_LITERAL_EXPR:
			free(self->number_literal.literal);
			break;
		case AST_INDEX_ACCESS_EXPR:
			expr_dispose(self->index_access.target);
			expr_dispose(self->index_access.index);
			free(self->index_access.target);
			free(self->index_access.index);
			break;
		case AST_CALL_EXPR:
			expr_dispose(self->index_access.target);
			for (i = 0; i < self->call.num_args; ++i)
				expr_dispose(self->call.args + i);
			free(self->index_access.target);
			free(self->call.args);
			break;
		case AST_MEMBER_ACCESS_EXPR:
			expr_dispose(self->member_access.target);
			free(self->member_access.target);
			free(self->member_access.name);
			break;
		case AST_INCDEC_EXPR:
			expr_dispose(self->incdec_op.target);
			free(self->incdec_op.target);
			break;
		case AST_UNARY_EXPR:
			expr_dispose(self->unary_op.target);
			free(self->unary_op.target);
			break;
		case AST_SIZEOF_EXPR:
			if (self->sizeof_op.mode == AST_EXPR_SIZEOF) {
				expr_dispose(self->sizeof_op.expr);
				free(self->sizeof_op.expr);
			}
			if (self->sizeof_op.mode == AST_TYPE_SIZEOF)
				type_dispose(&self->sizeof_op.type);
			break;
		case AST_FREE_BUILTIN:
			expr_dispose(self->free.expr);
			free(self->free.expr);
			break;
		case AST_NEW_BUILTIN:
			type_dispose(&self->newe.type);
			break;
		case AST_MAKE_BUILTIN:
			type_dispose(&self->make.type);
			expr_dispose(self->make.expr);
			free(self->make.expr);
			break;
		case AST_LENCAP_BUILTIN:
			expr_dispose(self->lencap.expr);
			free(self->lencap.expr);
			break;
		case AST_DISPOSE_BUILTIN:
			expr_dispose(self->dispose.expr);
			free(self->dispose.expr);
			break;
		case AST_CAST_EXPR:
			expr_dispose(self->cast.target);
			type_dispose(&self->cast.type);
			free(self->cast.target);
			break;
		case AST_BINARY_EXPR:
			expr_dispose(self->binary_op.lhs);
			expr_dispose(self->binary_op.rhs);
			free(self->binary_op.lhs);
			free(self->binary_op.rhs);
			break;
		case AST_CONDITIONAL_EXPR:
			expr_dispose(self->conditional.condition);
			expr_dispose(self->conditional.true_expr);
			expr_dispose(self->conditional.false_expr);
			free(self->conditional.condition);
			free(self->conditional.true_expr);
			free(self->conditional.false_expr);
			break;
		case AST_ASSIGNMENT_EXPR:
			expr_dispose(self->assignment.target);
			expr_dispose(self->assignment.expr);
			free(self->assignment.target);
			free(self->assignment.expr);
			break;
		case AST_COMMA_EXPR:
			for (i = 0; i < self->comma.num_exprs; ++i)
				expr_dispose(self->comma.exprs + i);
			free(self->comma.exprs);
			break;
		default:
			fprintf(stderr, "%s.%d: expr_dispose for expr kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}


void
stmt_dispose (stmt_t *self) {
	if (self == 0)
		return;

	unsigned i;
	switch (self->kind) {
		case AST_EXPR_STMT:
			expr_dispose(self->expr);
			break;
		case AST_COMPOUND_STMT:
			for (i = 0; i < self->compound.num_items; ++i)
				block_item_dispose(self->compound.items + i);
			free(self->compound.items);
			break;
		case AST_IF_STMT:
		case AST_SWITCH_STMT:
			expr_dispose(self->selection.condition);
			stmt_dispose(self->selection.stmt);
			stmt_dispose(self->selection.else_stmt);
			free(self->selection.condition);
			free(self->selection.stmt);
			free(self->selection.else_stmt);
			break;
		case AST_DO_STMT:
		case AST_FOR_STMT:
			expr_dispose(self->iteration.initial);
			expr_dispose(self->iteration.condition);
			expr_dispose(self->iteration.step);
			stmt_dispose(self->iteration.stmt);
			free(self->iteration.initial);
			free(self->iteration.condition);
			free(self->iteration.step);
			free(self->iteration.stmt);
			break;
		case AST_GOTO_STMT:
			free(self->name);
			break;
		case AST_CONTINUE_STMT:
			break;
		case AST_BREAK_STMT:
			break;
		case AST_RETURN_STMT:
			expr_dispose(self->expr);
			free(self->expr);
			break;
		case AST_LABEL_STMT:
			stmt_dispose(self->label.stmt);
			free(self->label.name);
			free(self->label.stmt);
			break;
		case AST_CASE_STMT:
			expr_dispose(self->label.expr);
			stmt_dispose(self->label.stmt);
			free(self->label.expr);
			free(self->label.stmt);
			break;
		case AST_DEFAULT_STMT:
			stmt_dispose(self->label.stmt);
			free(self->label.stmt);
			break;
		default:
			fprintf(stderr, "%s.%d: stmt_dispose for stmt kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}


void
block_item_dispose (block_item_t *self) {
	if (self == 0)
		return;

	switch (self->kind) {
		case AST_STMT_BLOCK_ITEM:
			stmt_dispose(self->stmt);
			free(self->stmt);
			break;
		case AST_DECL_BLOCK_ITEM:
			decl_dispose(self->decl);
			free(self->decl);
			break;
		default:
			fprintf(stderr, "%s.%d: block_item_dispose for block item kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}


void
decl_dispose (decl_t *self) {
	if (self == 0)
		return;

	unsigned i;
	switch (self->kind) {
		case AST_VARIABLE_DECL:
			type_dispose(&self->variable.type);
			expr_dispose(self->variable.initial);
			free(self->variable.name);
			free(self->variable.initial);
			break;
		case AST_CONST_DECL:
			type_dispose(self->cons.type);
			expr_dispose(&self->cons.value);
			free(self->cons.name);
			free(self->cons.type);
			break;
		case AST_IMPLEMENTATION_DECL:
			type_dispose(self->impl.interface);
			type_dispose(self->impl.target);
			free(self->impl.interface);
			free(self->impl.target);
			for (i = 0; i < self->impl.num_mappings; ++i) {
				free(self->impl.mappings[i].intf);
				free(self->impl.mappings[i].func);
			}
			free(self->impl.mappings);
			break;
		default:
			die("decl_dispose for decl kind %d not implemented", self->kind);
	}
}


char *
type_describe(type_t *self) {
	assert(self);
	char *s = 0;
	switch (self->kind) {
		case AST_VOID_TYPE:
			s = strdup("void");
			break;
		case AST_BOOLEAN_TYPE:
			s = strdup("bool");
			break;
		case AST_INTEGER_TYPE:
			asprintf(&s, "int%d", self->width);
			break;
		case AST_FLOAT_TYPE:
			asprintf(&s, "float%d", self->width);
			break;
		case AST_FUNC_TYPE: {
			char *sn;
			s = strdup("func(");

			unsigned i;
			for (i = 0; i < self->func.num_args; ++i) {
				char *arg = type_describe(self->func.args+i);
				asprintf(&sn, i > 0 ? "%s, %s" : "%s%s", s, arg);
				free(s);
				s = sn;
				free(arg);
			}

			char *ret = type_describe(self->func.return_type);
			asprintf(&sn, "%s) %s", s, ret);
			free(s);
			s = sn;
			free(ret);

			if (self->pointer > 0) {
				asprintf(&sn, "(%s)", s);
				free(s);
				s = sn;
			}
			break;
		}
		case AST_NAMED_TYPE:
			s = strdup(self->name);
			break;
		case AST_STRUCT_TYPE:
			s = strdup("struct{...}");
			break;
		case AST_ARRAY_TYPE: {
			char *t = type_describe(self->array.type);
			asprintf(&s, "[%d]%s", self->array.length,t);
			free(t);
			break;
		}
		case AST_SLICE_TYPE: {
			char *t = type_describe(self->slice.type);
			asprintf(&s, "[]%s", t);
			free(t);
			break;
		}
		case AST_INTERFACE_TYPE:
			if (self->interface.num_members == 0)
				s = strdup("interface{}");
			else
				s = strdup("interface{...}");
			break;
		case AST_PLACEHOLDER_TYPE:
			s = strdup("#");
			break;
		default:
			fprintf(stderr, "%s.%d: type_describe for type kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
	if (s && self->pointer > 0) {
		char suffix[self->pointer+1];
		memset(suffix, '*', self->pointer);
		suffix[self->pointer] = 0;
		char *sn = 0;
		asprintf(&sn, "%s%s", s, suffix);
		free(s);
		s = sn;
	}
	return s;
}

void
type_copy (type_t *dst, const type_t *src) {
	assert(dst);
	assert(src);
	*dst = *src;
	unsigned i,n;
	switch (src->kind) {
		case AST_NO_TYPE:
		case AST_VOID_TYPE:
		case AST_BOOLEAN_TYPE:
		case AST_INTEGER_TYPE:
		case AST_FLOAT_TYPE:
		case AST_PLACEHOLDER_TYPE:
			break;
		case AST_FUNC_TYPE:
			dst->func.return_type = malloc(sizeof(type_t));
			type_copy(dst->func.return_type, src->func.return_type);
			dst->func.args = malloc(src->func.num_args * sizeof(type_t));
			for (i = 0; i < src->func.num_args; ++i)
				type_copy(dst->func.args+i, src->func.args+i);
			break;
		case AST_NAMED_TYPE:
			dst->name = strdup(src->name);
			break;
		case AST_STRUCT_TYPE:
			dst->strct.members = malloc(src->strct.num_members * sizeof(struct_member_t));
			for (i = 0; i < src->strct.num_members; ++i) {
				dst->strct.members[i].type = malloc(sizeof(type_t));
				type_copy(dst->strct.members[i].type, src->strct.members[i].type);
				dst->strct.members[i].name = strdup(src->strct.members[i].name);
			}
			break;
		case AST_ARRAY_TYPE:
			type_copy(dst->array.type, src->array.type);
			break;
		case AST_SLICE_TYPE:
			type_copy(dst->slice.type, src->slice.type);
			break;
		case AST_INTERFACE_TYPE:
			dst->interface.members = malloc(src->interface.num_members * sizeof(interface_member_t));
			for (i = 0; i < src->interface.num_members; ++i) {
				interface_member_t *md = dst->interface.members+i;
				interface_member_t *ms = src->interface.members+i;
				switch (ms->kind) {
					case AST_MEMBER_FIELD:
						md->field.name = strdup(ms->field.name);
						md->field.type = malloc(sizeof(type_t));
						type_copy(md->field.type, ms->field.type);
						break;
					case AST_MEMBER_FUNCTION:
						md->func.name = strdup(ms->func.name);
						md->func.return_type = malloc(sizeof(type_t));
						type_copy(md->func.return_type, ms->func.return_type);
						md->func.args = malloc(sizeof(type_t) * ms->func.num_args);
						for (n = 0; n < ms->func.num_args; ++n)
							type_copy(md->func.args+n, ms->func.args+n);
						break;
					default:
						fprintf(stderr, "%s.%d: type_copy for interface member kind %d not implemented\n", __FILE__, __LINE__, ms->kind);
						abort();
				}
			}
			break;
		default:
			fprintf(stderr, "%s.%d: type_copy for type kind %d not implemented\n", __FILE__, __LINE__, src->kind);
			abort();
	}
}

int
type_equal (const type_t *a, const type_t *b) {
	if (a->kind != b->kind ||
		a->pointer != b->pointer)
		return 0;

	unsigned i,n;
	switch (a->kind) {
		case AST_NO_TYPE:
		case AST_VOID_TYPE:
		case AST_BOOLEAN_TYPE:
		case AST_PLACEHOLDER_TYPE:
			return 1;
		case AST_INTEGER_TYPE:
		case AST_FLOAT_TYPE:
			return a->width == b->width;
		case AST_NAMED_TYPE:
			return strcmp(a->name, b->name) == 0;
		case AST_FUNC_TYPE: {
			if (a->func.num_args != b->func.num_args ||
				!type_equal(a->func.return_type, b->func.return_type))
				return 0;
			for (i = 0; i < a->func.num_args; ++i)
				if (!type_equal(a->func.args+i, b->func.args+i))
					return 0;
			return 1;
		}
		case AST_STRUCT_TYPE: {
			if (a->strct.num_members != b->strct.num_members)
				return 0;
			for (i = 0; i < a->strct.num_members; ++i)
				if (strcmp(a->strct.members[i].name, b->strct.members[i].name) != 0 ||
					!type_equal(a->strct.members[i].type, b->strct.members[i].type))
					return 0;
			return 1;
		}
		case AST_SLICE_TYPE: {
			return type_equal(a->slice.type,b->slice.type);
		}
		case AST_ARRAY_TYPE:
			return a->array.length == b->array.length || type_equal(a->array.type, b->array.type);
		case AST_INTERFACE_TYPE: {
			if (a->interface.num_members != b->interface.num_members)
				return 0;
			for (i = 0; i < a->interface.num_members; ++i) {
				interface_member_t *ma = a->interface.members+i;
				interface_member_t *mb = b->interface.members+i;
				if (ma->kind != mb->kind)
					return 0;
				switch (ma->kind) {
					case AST_MEMBER_FIELD:
						if (strcmp(ma->field.name, mb->field.name) != 0) return 0;
						if (!type_equal(ma->field.type, mb->field.type)) return 0;
						break;
					case AST_MEMBER_FUNCTION:
						if (ma->func.num_args != mb->func.num_args) return 0;
						if (strcmp(ma->func.name, mb->func.name) != 0) return 0;
						if (!type_equal(ma->func.return_type, mb->func.return_type)) return 0;
						for (n = 0; n < ma->func.num_args; ++n)
							if (!type_equal(ma->func.args+n, mb->func.args+n))
								return 0;
						break;
					default:
						fprintf(stderr, "%s.%d: type_equal for interface member kind %d not implemented\n", __FILE__, __LINE__, ma->kind);
						abort();
				}
			}
			return 1;
		}
		default:
			fprintf(stderr, "%s.%d: type_equal for type kind %d not implemented\n", __FILE__, __LINE__, a->kind);
			abort();
	}
}

void
type_dispose (type_t *self) {
	if (self == 0)
		return;
	unsigned i,n;
	switch (self->kind) {
		case AST_VOID_TYPE:
		case AST_BOOLEAN_TYPE:
		case AST_INTEGER_TYPE:
		case AST_FLOAT_TYPE:
		case AST_PLACEHOLDER_TYPE:
			break;
		case AST_FUNC_TYPE:
			type_dispose(self->func.return_type);
			for (i = 0; i < self->func.num_args; ++i)
				type_dispose(self->func.args + i);
			free(self->func.return_type);
			free(self->func.args);
			break;
		case AST_NAMED_TYPE:
			free(self->name);
			break;
		case AST_STRUCT_TYPE:
			for (i = 0; i < self->strct.num_members; ++i) {
				type_dispose(self->strct.members[i].type);
				free(self->strct.members[i].type);
				free(self->strct.members[i].name);
			}
			free(self->strct.members);
			break;
		case AST_ARRAY_TYPE:
			type_dispose(self->array.type);
			free(self->array.type);
			break;
		case AST_SLICE_TYPE:
			type_dispose(self->slice.type);
			free(self->slice.type);
			break;
		case AST_INTERFACE_TYPE:
			for (i = 0; i < self->interface.num_members; ++i) {
				interface_member_t *m = self->interface.members+i;
				switch (m->kind) {
					case AST_MEMBER_FIELD:
						type_dispose(m->field.type);
						free(m->field.name);
						free(m->field.type);
						break;
					case AST_MEMBER_FUNCTION:
						type_dispose(m->func.return_type);
						for (n = 0; n < m->func.num_args; ++n)
							type_dispose(m->func.args+n);
						free(m->func.name);
						free(m->func.return_type);
						free(m->func.args);
						break;
					default:
						die("type_dispose for interface member kind %d not implemented", m->kind);
				}
			}
			free(self->interface.members);
			break;
		default:
			die("type_dispose for type kind %d not implemented", self->kind);
	}
}


void
unit_dispose (unit_t *self) {
	if (self == 0)
		return;
	unsigned i;
	switch (self->kind) {
		case AST_IMPORT_UNIT:
			free(self->import_name);
			break;
		case AST_DECL_UNIT:
			decl_dispose(self->decl);
			free(self->decl);
			break;
		case AST_FUNC_UNIT:
			type_dispose(&self->func.return_type);
			free(self->func.name);
			stmt_dispose(self->func.body);
			free(self->func.body);
			for (i = 0; i < self->func.num_params; ++i) {
				type_dispose(&self->func.params[i].type);
				free(self->func.params[i].name);
			}
			free(self->func.params);
			break;
		case AST_TYPE_UNIT:
			type_dispose(&self->type.type);
			free(self->type.name);
			break;
		default:
			fprintf(stderr, "%s.%d: unit_dispose for unit kind %d not implemented\n", __FILE__, __LINE__, self->kind);
			abort();
			break;
	}
}
