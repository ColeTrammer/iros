#ifndef _MAPPING_H
#define _MAPPING_H 1

#include <netinet/in.h>

struct host_mapping {
    struct host_mapping *next;
    struct host_mapping *prev;

    struct in_addr ip;
    char *name;
    char **aliases;
};

struct host_mapping *get_known_hosts();
struct host_mapping *lookup_host_in_mapping(struct host_mapping *list, const char *name);

#endif /* _MAPPING_H */
