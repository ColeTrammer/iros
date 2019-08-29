#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/fs/fs_manager.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/proc/process.h>
#include <kernel/proc/elf64.h>

void load_process(const char *file_name) {
    VFILE *program = fs_open(file_name);
    fs_seek(program, 0, SEEK_END);
    long length = fs_tell(program);
    fs_seek(program, 0, SEEK_SET);

    void *buffer = malloc(length);
    fs_read(program, buffer, length);

    uint64_t types[] = { VM_PROCESS_TEXT, VM_PROCESS_ROD, VM_PROCESS_DATA, VM_PROCESS_BSS };
    for (size_t i = 0; i < 4; i++) {
        uint64_t type = types[i];
        struct vm_region *region = elf64_create_vm_region(buffer, type);
        add_vm(region);
        map_vm_region(region);

        if (type == VM_PROCESS_BSS) {
            memset((void*) region->start, 0, region->end - region->start);
        }
    }

    memcpy((void*) elf64_get_start(buffer), buffer, length);
}