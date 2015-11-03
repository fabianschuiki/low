/* Copyright (c) 2015 Fabian Schuiki */
#include "grammar.h"
#include "lexer.h"
#include <stdio.h>
#include <string.h>

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
	#define VAR_END(reducer)
#define RULE_END

GRAMMAR

#undef RULE
#undef VAR
#undef TKN
#undef SUB
#undef VAR_END
#undef RULE_END


// Definitions.
#define RULE(name) const rule_t SYMNAME(name) = { #name, (const variant_t[]){
	#define VAR {(const rule_t*[]){
		#define TKN(name) (void*)TKN_##name,
		#define SUB(name) &SYMNAME(name),
	#define VAR_END(reducer) 0}, reducer},
#define RULE_END {0}}};

GRAMMAR

#undef RULE
#undef VAR
#undef TKN
#undef SUB
#undef VAR_END
#undef RULE_END

// Root rule.
const rule_t grammar_root = {
	"root", (const variant_t[]){
		{(const rule_t*[]){&SYMNAME(GRAMMAR_ROOT), 0}, 0},
		{0},
	}
};


#undef GRAMMAR
#undef GRAMMAR_ROOT
