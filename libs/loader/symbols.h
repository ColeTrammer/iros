#ifndef _SYMBOLS_H
#define _SYMBOLS_H 1

#include <bits/dynamic_elf_object.h>

#include "loader.h"

struct Elf64_Sym;

#define SYMBOL_LOOKUP_NOT_CURRENT 1

struct symbol_lookup_result do_symbol_lookup(const char *s, const struct dynamic_elf_object *current_object, int flags) LOADER_PRIVATE;

#endif /* _SYMBOLS_H */
