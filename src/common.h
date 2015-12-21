/* Copyright (c) 2015 Fabian Schuiki */
#pragma once

void die_impl(const char *filename, unsigned line, const char *fmt, ...);
#define die(fmt, ...) die_impl(__FILE__, __LINE__, fmt, __VA_ARGS__)
