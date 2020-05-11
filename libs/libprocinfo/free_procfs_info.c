#include <procinfo.h>
#include <stdlib.h>

void free_procfs_info(struct proc_info *info) {
#ifndef __os_2__
    (void) info;
#else
    free(info);
#endif /* __os_2__ */
}
