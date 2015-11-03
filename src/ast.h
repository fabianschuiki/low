/* Copyright (c) 2015 Fabian Schuiki */
#pragma once

typedef struct func_def func_def_t;

struct func_def {
	char *name;
	unsigned flags;
	unsigned num_inputs;
	void *inputs;
};

enum func_def_flags {
	FUNC_DEF_STATIC = 1 << 0,
	FUNC_DEF_EXTERN = 1 << 1,
};

void func_def_dispose(func_def_t *self);
