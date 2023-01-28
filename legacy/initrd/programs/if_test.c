#include <net/if.h>
#include <stddef.h>
#include <stdio.h>

int main() {
    struct if_nameindex *ni = if_nameindex();
    if (!ni) {
        perror("if_test: if_nameindex");
        return 1;
    }

    for (size_t i = 0; ni[i].if_index != 0 || ni[i].if_name != NULL; i++) {
        printf("INTERFACE: %u => %s\n", ni[i].if_index, ni[i].if_name);
    }

    if_freenameindex(ni);

    char name_buffer[IF_NAMESIZE];
    char *name = if_indextoname(1, name_buffer);
    if (!name) {
        perror("if_test: if_indextoname");
        return 1;
    }

    printf("GOT INTERFACE: %u => %s\n", 1, name);

    unsigned int index = if_nametoindex("lo");
    if (index == 0) {
        perror("if_test: if_nametoindex");
        return 1;
    }

    printf("GOT INTERFACE: %s => %u\n", "lo", index);
    return 0;
}
