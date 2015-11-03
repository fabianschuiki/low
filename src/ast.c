/* Copyright (c) 2015 Fabian Schuiki */
#include "ast.h"
#include <stdlib.h>

void func_def_dispose(func_def_t *self) {
	if (self->name)
		free(self->name);
}
