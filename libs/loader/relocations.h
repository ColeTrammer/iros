#ifndef _RELOCATIONS_H
#define _RELOCATIONS_H 1

#include "loader.h"

void process_relocations(const struct dynamic_elf_object *self) LOADER_PRIVATE;

#endif /* _RELOCATIONS_H */
