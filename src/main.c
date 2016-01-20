/* Copyright (c) 2015-2016 Fabian Schuiki */
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "options.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Linker.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


static array_t *
parse_file (const char *filename) {
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return 0;
	}

	struct stat fs;
	if (fstat(fd, &fs) == -1) {
		perror("fstat");
		close(fd);
		return 0;
	}

	if (!S_ISREG(fs.st_mode)) {
		fprintf(stderr, "not a regular file\n");
		close(fd);
		return 0;
	}

	char *p = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return 0;
	}

	if (close(fd) == -1) {
		perror("close");
		return 0;
	}

	// printf("compiling %s\n", filename);
	lexer_t lex;
	lexer_init(&lex, p, fs.st_size, filename);
	lexer_next(&lex);
	array_t *units = parse(&lex);
	assert(lex.token == TKN_EOF && "lexer did not consume entire file");

	if (munmap(p, fs.st_size) == -1) {
		perror("munmap");
		return 0;
	}

	return units;
}


static void
resolve_imports (codegen_t *cg, codegen_context_t *context, const array_t *units, const char *relative_to, array_t *handled) {

	// Determine the basename of relative_to.
	const char *last_slash = strrchr(relative_to, '/');
	unsigned basename_len = last_slash ? last_slash-relative_to : 0;
	char basename[basename_len+1];
	strncpy(basename, relative_to, basename_len);
	basename[basename_len] = 0;

	unsigned i;
	for (i = 0; i < units->size; ++i) {
		unit_t *unit = array_get(units,i);
		if (unit->kind == AST_IMPORT_UNIT) {
			char *filename;
			if (*basename)
				asprintf(&filename, "%s/%s", basename, unit->import_name);
			else
				filename = strdup(unit->import_name);

			// Skip imports that have already been handled.
			unsigned n;
			for (n = 0; n < handled->size; ++n)
				if (strcmp(filename, *(char**)array_get(handled,n)) == 0)
					break;
			if (n < handled->size) {
				free(filename);
				continue;
			}
			array_add(handled, &filename);

			array_t *imported_units = parse_file(filename);
			if (!imported_units) {
				fprintf(stderr, "unable to open file %s imported from %s\n", filename, relative_to);
				abort();
			}
			resolve_imports(cg, context, imported_units, filename, handled);
			codegen_decls(cg, context, imported_units);
		}
	}
}


static int
compile (const char *inname, const char *outname, LLVMModuleRef *module) {
	unsigned i;

	// Parse the file.
	array_t *units = parse_file(inname);
	if (!units)
		return 1;

	// Prepare the codegen context and module that will hold the code of this
	// file.
	codegen_t cg;
	codegen_context_t ctx;
	bzero(&cg, sizeof cg);
	codegen_context_init(&ctx);
	cg.module = LLVMModuleCreateWithName(inname);

	// Resolve all imports, populating the codegen context with the declarations
	// of the imported files.
	array_t handled_imports;
	array_init(&handled_imports, sizeof(char*));
	resolve_imports(&cg, &ctx, units, inname, &handled_imports);
	for (i = 0; i < handled_imports.size; ++i)
		free(*(char**)array_get(&handled_imports,i));
	array_dispose(&handled_imports);

	// Generate the code for this file.
	codegen(&cg, &ctx, units);

	char *error = NULL;
	LLVMVerifyModule(cg.module, LLVMAbortProcessAction, &error);
	LLVMDisposeMessage(error);

	// Write the generated LLVM IR to disk.
	// LLVMWriteBitcodeToFile(cg.module, "parsed.bc");
	error = NULL;
	LLVMPrintModuleToFile(cg.module, outname, &error);
	LLVMDisposeMessage(error);

	// Return the module or get rid of it.
	if (module)
		*module = cg.module;
	else
		LLVMDisposeModule(cg.module);

	// Clean up.
	codegen_context_dispose(&ctx);
	for (i = 0; i < units->size; ++i)
		unit_dispose(array_get(units,i));
	array_dispose(units);
	free(units);

	return 0;
}


int
main (int argc, char **argv) {

	// Parse command line options.
	parse_options(&argc, argv);

	// Compile input files.
	if (argc == 1) {
		fprintf(stderr, "no input files\n");
		return(1);
	}

	int any_failed = 0;
	unsigned num_modules = argc-1;
	LLVMModuleRef modules[num_modules];
	unsigned i;
	for (i = 0; i < argc-1; ++i) {
		char *arg = argv[i+1];

		// Determine the output file name.
		const char *last_slash = strrchr(arg, '/');
		const char *last_dot = strrchr(last_slash ? last_slash : arg, '.');
		unsigned basename_len = (last_dot ? last_dot-arg : strlen(arg));
		char out_name[basename_len+4];
		strncpy(out_name, arg, basename_len);
		strcpy(out_name+basename_len, ".ll");

		// Compile the file.
		int err = compile(arg, out_name, modules+i);
		if (err) {
			fprintf(stderr, "unable to compile %s\n", arg);
			any_failed = 1;
			if (modules[i]) {
				LLVMDisposeModule(modules[i]);
				modules[i] = 0;
			}
		}
	}

	if (any_failed)
		return 1;

	// Link the compiled files into an output file, if so requested.
	if (options.output_name) {
		for (i = 1; i < num_modules; ++i) {
			char *error = 0;
			LLVMBool failed = LLVMLinkModules(modules[0], modules[i], 0, &error);
			if (failed) {
				fprintf(stderr, "unable to link %s: %s\n", argv[i+1], error);
				return 1;
			}
			LLVMDisposeMessage(error);
		}

		char *error = 0;
		LLVMPrintModuleToFile(modules[0], options.output_name, &error);
		LLVMDisposeMessage(error);
		LLVMDisposeModule(modules[0]);
	} else {
		for (i = 0; i < num_modules; ++i)
			LLVMDisposeModule(modules[i]);
	}

	return any_failed;
}
