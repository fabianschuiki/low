/* Copyright (c) 2015-2016 Fabian Schuiki, Thomas Richner */
#include "grammar.h"
#include "lexer.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


static void
dformat(loc_t *loc, const char *prefix, const char *fmt, va_list ap) {
	if (loc && loc->filename)
		printf("%s:%d:%d: ", loc->filename, loc->line+1, loc->col+1);
	printf("%s", prefix);
	vprintf(fmt, ap);
}

void
dinfo(loc_t *loc, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	dformat(loc, "\033[32;1minfo:\033[0m ", fmt, ap);
	va_end(ap);
}

void
derror(loc_t *loc, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	dformat(loc, "\033[31;1merror:\033[0m ", fmt, ap);
	va_end(ap);
	exit(1);
}


// Include the rules that are defined in a separate file.
#include "grammar_rules.h"

// A macro that translates rule names into symbol names within the executable.
// Note that the *_INNER macro is required such that passing a macro to SYMNAME
// properly expands that macro first.
#define SYMNAME_INNER(name) grammar_##name
#define SYMNAME(name) SYMNAME_INNER(name)


// Declarations.
#define RULE(name) extern const rule_t SYMNAME(name);
	#define VAR
		#define TKN(name)
		#define SUB(name)
	#define REDUCE(reducer)
	#define REDUCE_TAG(reducer,tag)
	#define REDUCE_DEFAULT
#define RULE_END

GRAMMAR

#undef RULE
#undef VAR
#undef TKN
#undef SUB
#undef REDUCE
#undef REDUCE_TAG
#undef REDUCE_DEFAULT
#undef RULE_END


// Definitions.
#define RULE(name) const rule_t SYMNAME(name) = { #name, (const variant_t[]){
	#define VAR {(const rule_t*[]){
		#define TKN(name) (void*)TKN_##name,
		#define SUB(name) &SYMNAME(name),
	#define REDUCE(reducer) 0}, "reduce_" #reducer, 0},
	#define REDUCE_TAG(reducer,tag) 0}, "reduce_" #reducer, tag},
	#define REDUCE_DEFAULT 0}, 0},
#define RULE_END {0}}};

GRAMMAR

#undef RULE
#undef VAR
#undef TKN
#undef SUB
#undef REDUCE
#undef REDUCE_TAG
#undef REDUCE_DEFAULT
#undef RULE_END

// Root rule.
const rule_t grammar_root = {
	"root", (const variant_t[]){
		{(const rule_t*[]){&SYMNAME(GRAMMAR_ROOT), 0}, "0 /* accept */"},
		{0},
	}
};


#undef GRAMMAR
#undef GRAMMAR_ROOT
