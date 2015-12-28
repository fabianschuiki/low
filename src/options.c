/* Copyright (c) 2015 Fabian Schuiki */
#include "options.h"
#include <stdio.h>
#include <stdlib.h>


struct options options = { 0 };


/// Parse a short (i.e. single hyphen) option. \a opt is the option character
/// without the leading hyphen, i.e. "v" if the option was "-v". The arguments
/// \a argv and \a arge point to the current and the last input argument,
/// respectively. An option may consume additional arguments by incrementing
/// \c *argv and verifying that the new pointer does not lie beyond \c arge.
void
parse_short_option(char opt, char ***argv, char **arge) {
	switch (opt) {
		case 'o': {
			++*argv;
			if (*argv == arge) {
				fprintf(stderr, "expected output file name after -o\n");
				exit(1);
			}
			options.output_name = **argv;
		} break;

		default:
			fprintf(stderr, "unknown option -%c\n", opt);
			exit(1);
	}
}


/// Parse a long (i.e. double hyphen) option. \a opt is the option text without
/// the leading double hyphen, i.e. "verbose" if the option was "--verbose". The
/// arguments \a argv and \a arge point to the current and the last input
/// argument, respectively. An option may consume additional arguments by
/// incrementing \c *argv and verifying that the new pointer does not lie beyond
/// \c arge.
void
parse_long_option(char *opt, char ***argv, char **arge) {
	fprintf(stderr, "unknown option --%s\n", opt);
	exit(1);
}


/// Parse a list of command line arguments, removing any recognized options from
/// the list. The argument count \a argc is modified to represent the number of
/// positional arguments left in the list, and the arguments \a argv are shifted
/// such that after parsing, only the positional arguments are left. Calls
/// parse_long_option and parse_short_option to handle the corresponding
/// options. Note that the first argument is assumed to be the command used to
/// execute lowc, and is left intact.
void
parse_options(int *argc, char **argv) {
	char **argi = argv+1, **argo = argv+1, **arge = argv+*argc;
	*argc = 1;
	for (; argi != arge; ++argi) {
		char *arg = *argi;

		// If the argument starts with a hyphen, treat it as a potential option.
		if (arg[0] == '-') {
			// Double hyphens make for long options, single hyphens make for
			// short options which may be concatenated (-ab instead of -a -b).
			if (arg[1] == '-') {
				// A simple double hyphen terminates option parsing and treats
				// the remainder of the arguments as positional.
				if (arg[2] == 0) {
					++argi;
					break;
				} else {
					parse_long_option(arg+2, &argi, arge);
				}
			} else {
				char *c;
				for (c = arg+1; *c; ++c)
					parse_short_option(*c, &argi, arge);
			}
		} else {
			*argo++ = *argi;
		}
	}
	while (argi != arge)
		*argo++ = *argi++;
	*argc = argo-argv;
}
