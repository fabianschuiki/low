/* Copyright (c) 2015 Thomas Richner, Fabian Schuiki */
#include "codegen_internal.h"

#define CODEGEN_TYPE(name) LLVMTypeRef codegen_type_##name(codegen_context_t *context, type_t *type)

static codegen_symbol_t*
context_find_local (codegen_context_t *self, const char *name) {
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

static LLVMTypeRef
make_interface_table_type (codegen_context_t *context, interface_member_t *members, unsigned num_members) {
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

CODEGEN_TYPE(void){
	return LLVMVoidType();
}

CODEGEN_TYPE(boolean){
	return LLVMInt1Type();
}

CODEGEN_TYPE(integer){
	return LLVMIntType(type->width);
}

CODEGEN_TYPE(float){
	switch (type->width) {
		case 16: return LLVMHalfType();
		case 32: return LLVMFloatType();
		case 64: return LLVMDoubleType();
		case 128: return LLVMFP128Type();
	}
	die("floating point type must be 16, 32, 64 or 128 bits wide but was %d\n",type->width);
	return NULL;
}

CODEGEN_TYPE(func){
	LLVMTypeRef args[type->func.num_args];
	unsigned i;
	for (i = 0; i < type->func.num_args; ++i)
		args[i] = codegen_type(context, &type->func.args[i]);
	return LLVMFunctionType(codegen_type(context, type->func.return_type), args, type->func.num_args, 0);
}

CODEGEN_TYPE(named){
	codegen_symbol_t *sym = context_find_local(context, type->name);
	if (!sym)
		derror(0, "unknown type name '%s'\n", type->name);
	if (!sym->type)
		derror(0, "'%s' is not a type name\n", type->name);
	// assert(sym && "unknown type name");
	return codegen_type(context, sym->type);
}

CODEGEN_TYPE(struct){
	LLVMTypeRef members[type->strct.num_members];
	unsigned i;
	for (i = 0; i < type->strct.num_members; ++i)
		members[i] = codegen_type(context, type->strct.members[i].type);
	return LLVMStructType(members, type->strct.num_members, 0);
}

CODEGEN_TYPE(slice){
	// underlying struct of a slice
	LLVMTypeRef arrtype = LLVMPointerType(LLVMArrayType(codegen_type(context, type->slice.type),0),0);
	LLVMTypeRef members[4];
	members[0] = arrtype; // pointer to array
	members[1] = LLVMIntType(64); 			// length @HARDCODED
	members[2] = LLVMIntType(64); 			// capacity @HARDCODED
	members[3] = arrtype; // base
	return LLVMStructType(members, 4, 0); 	// NOT PACKED
}

CODEGEN_TYPE(array){
	LLVMTypeRef element = codegen_type(context, type->array.type);
	return LLVMArrayType(element, type->array.length);
}

CODEGEN_TYPE(interface){
	LLVMTypeRef fields[] = {
		LLVMPointerType(make_interface_table_type(context, type->interface.members, type->interface.num_members), 0),
		LLVMPointerType(LLVMInt8Type(), 0), // pointer to the object
	};
	unsigned i;
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

const codegen_type_fn_t codegen_type_fn[AST_NUM_TYPES] = {
	[AST_VOID_TYPE]     		= codegen_type_void,
	[AST_BOOLEAN_TYPE]     		= codegen_type_boolean,
	[AST_INTEGER_TYPE]     		= codegen_type_integer,
	[AST_FLOAT_TYPE]     		= codegen_type_float,
	[AST_FUNC_TYPE]     		= codegen_type_func,
	[AST_NAMED_TYPE]     		= codegen_type_named,
	[AST_STRUCT_TYPE]     		= codegen_type_struct,
	[AST_SLICE_TYPE]     		= codegen_type_slice,
	[AST_ARRAY_TYPE]     		= codegen_type_array,
	[AST_INTERFACE_TYPE]     	= codegen_type_interface,

};

LLVMTypeRef
codegen_type(codegen_context_t *context, type_t *type) {
	assert(type);
	if (type->pointer > 0) {
		type_t inner = *type;
		--inner.pointer;
		return LLVMPointerType(codegen_type(context, &inner), 0);
	}

	codegen_type_fn_t fn = codegen_type_fn[type->kind];

	if(!fn){
		die("type codegen for kind %d not implemented", type->kind);
	}

	return fn(context, type);
}


