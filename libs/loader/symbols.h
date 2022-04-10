#ifndef _SYMBOLS_H
#define _SYMBOLS_H 1

#include <bits/dynamic_elf_object.h>

#include "loader.h"

#define SYMBOL_LOOKUP_NOT_CURRENT 1

struct symbol_lookup_result do_symbol_lookup(const char *s, const struct dynamic_elf_object *current_object, int flags) LOADER_PRIVATE;
struct symbol_lookup_result do_addr_lookup(void *addr);

#endif /* _SYMBOLS_H */
