#include <net/if.h>
#include <stdlib.h>

void if_freenameindex(struct if_nameindex *ptr) {
    for (size_t i = 0; ptr[i].if_index != 0 || ptr[i].if_name != NULL; i++) {
        free(ptr[i].if_name);
    }
    free(ptr);
}
