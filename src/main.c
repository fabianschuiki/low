/* Copyright (c) 2015 Fabian Schuiki */
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
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


static void
parse_options (int *argc, char **argv) {
	char **argi = argv+1;
	char **argo = argv+1;
	char **arge = argv+*argc;
	for (; argi != arge; ++argi) {
		if (*argi[0] == '-') {
			if (strcmp(*argi, "--") == 0) {
				++argi;
				for (; argi != arge; ++argi)
					*argo++ = *argi;
				break;
			} else {
				// TODO: Parse option
				fprintf(stderr, "unknown option %s\n", *argi);
				exit(1);
			}
		} else {
			*argo++ = *argi;
		}
	}
	*argc = argo-argv;
}


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
	lexer_init(&lex, p, fs.st_size);
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
compile (const char *filename) {
	unsigned i;

	const char *last_slash = strrchr(filename, '/');
	const char *last_dot = strrchr(last_slash ? last_slash : filename, '.');
	unsigned basename_len = (last_dot ? last_dot-filename : strlen(filename));
	char out_name[basename_len+4];
	strncpy(out_name, filename, basename_len);
	strcpy(out_name+basename_len, ".ll");

	array_t *units = parse_file(filename);
	if (!units)
		return 1;

	codegen_t cg;
	codegen_context_t ctx;
	bzero(&cg, sizeof cg);
	codegen_context_init(&ctx);
	cg.module = LLVMModuleCreateWithName(filename);

	array_t handled_imports;
	array_init(&handled_imports, sizeof(char*));
	resolve_imports(&cg, &ctx, units, filename, &handled_imports);
	for (i = 0; i < handled_imports.size; ++i)
		free(*(char**)array_get(&handled_imports,i));
	array_dispose(&handled_imports);

	codegen(&cg, &ctx, units);

	char *error = NULL;
	LLVMVerifyModule(cg.module, LLVMAbortProcessAction, &error);
	LLVMDisposeMessage(error);

	// LLVMWriteBitcodeToFile(cg.module, "parsed.bc");
	error = NULL;
	LLVMPrintModuleToFile(cg.module, out_name, &error);
	LLVMDisposeMessage(error);

	LLVMDisposeModule(cg.module);

	// printf("after compilation, the context contains %d symbols:\n", ctx.symbols.size);
	// for (i = 0; i < ctx.symbols.size; ++i) {
	// 	codegen_symbol_t *sym = array_get(&ctx.symbols,i);
	// 	printf("  <%s>", sym->name);
	// 	if (sym->type) {
	// 		char *buf = type_describe((type_t*)sym->type);
	// 		printf("  %s", buf);
	// 		free(buf);
	// 	}
	// 	printf("\n");
	// }

	codegen_context_dispose(&ctx);

	for (i = 0; i < units->size; ++i)
		unit_dispose(array_get(units,i));
	array_dispose(units);
	free(units);

	return 0;
}


int
main (int argc, char **argv) {
	/// TODO: Open the file using mmap and perform the lexical analysis.
	/// TODO: Parse the input into an AST.
	/// TODO: Iteratively elaborate the code, executing compile-time code where necessary.
	/// TODO: Generate code.

	parse_options(&argc, argv);

	if (argc == 1) {
		fprintf(stderr, "no input files\n");
		return(1);
	}

	char **arg;
	int any_failed = 0;
	for (arg = argv+1; arg != argv+argc; ++arg) {
		int err = compile(*arg);
		if (err) {
			printf("unable to compile %s\n", *arg);
			any_failed = 1;
		}
	}

	return any_failed;
}
