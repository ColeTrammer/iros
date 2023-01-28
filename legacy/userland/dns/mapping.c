#include <assert.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>

#include "mapping.h"

struct host_mapping *get_known_hosts() {
    struct host_mapping *hosts = NULL;

    struct hostent *ent;
    while ((ent = gethostent())) {
        assert(ent->h_addrtype == AF_INET);
        assert(ent->h_length == sizeof(struct in_addr));

        struct host_mapping *new_host = malloc(sizeof(struct host_mapping));
        new_host->next = hosts;
        new_host->prev = NULL;
        new_host->ip = *(struct in_addr *) ent->h_addr_list[0];
        new_host->name = strdup(ent->h_name);

        size_t aliases_length = 0;
        while (ent->h_aliases[aliases_length]) {
            aliases_length++;
        }
        new_host->aliases = malloc((aliases_length + 1) * sizeof(char *));
        for (size_t i = 0; i < aliases_length; i++) {
            new_host->aliases[i] = strdup(ent->h_aliases[i]);
        }
        new_host->aliases[aliases_length] = NULL;

        hosts = new_host;
    }

    endhostent();
    return hosts;
}

struct host_mapping *lookup_host_in_mapping(struct host_mapping *list, const char *name) {
    for (struct host_mapping *iter = list; iter; iter = iter->next) {
        if (strcmp(iter->name, name) == 0) {
            return iter;
        }
        for (size_t i = 0; iter->aliases[i]; i++) {
            if (strcmp(iter->aliases[i], name) == 0) {
                return iter;
            }
        }
    }

    return NULL;
}
