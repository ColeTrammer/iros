#include <stdint.h>
#include <stdio.h>

#include <kernel/fs/fs_manager.h>
#include <kernel/fs/initrd.h>
#include <kernel/mem/vm_region.h>
#include <kernel/mem/vm_allocator.h>

void init_fs_manager() {
    struct vm_region *initrd = find_vm_region(VM_INITRD);
    uintptr_t start = initrd->start;

    int64_t num_files = *((int64_t*) start);
    printf("Num Files: %ld\n", num_files);

    struct initrd_file_entry *entry = (struct initrd_file_entry*) (start + sizeof(int64_t));
    for (int i = 0; i < num_files; i++) {
        printf("Name: %s | Off: %#.8X | Size: %#.8X\n", entry->name, entry->offset, entry->length);
        printf("%s\n", start + entry->offset);
        entry++;
    }
}