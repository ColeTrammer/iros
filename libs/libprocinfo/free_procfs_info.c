#include <procinfo.h>
#include <stdlib.h>

void free_procfs_info(struct proc_info *info) {
    free(info);
}