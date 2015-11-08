/* Copyright (c) 2015 Fabian Schuiki */
#pragma once
#include "array.h"
#include <llvm-c/Core.h>

void codegen(LLVMModuleRef module, const array_t *units);
