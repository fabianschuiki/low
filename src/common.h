/* Copyright (c) 2015-2016 Fabian Schuiki */
#pragma once

void die_impl(const char *filename, unsigned line, const char *fmt, ...);
#define die(...) die_impl(__FILE__, __LINE__, __VA_ARGS__)
