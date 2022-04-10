#include <elf.h>

#include "../../dynamic_elf_object.h"
#include "../../relocations.h"
#include "../../symbols.h"
#include "../../tls_record.h"

int do_process_relocations(struct dynamic_elf_object *self, bool bind_now) {
    (void) self;
    (void) bind_now;
    return 0;
}
