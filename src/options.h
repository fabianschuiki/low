/* Copyright (c) 2015-2016 Fabian Schuiki */
#pragma once

extern struct options {
	char *output_name;
} options;

void parse_short_option(char opt, char ***argv, char **arge);
void parse_long_option(char *opt, char ***argv, char **arge);
void parse_options(int *argc, char **argv);
