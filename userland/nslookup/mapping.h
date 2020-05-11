#ifndef _MAPPING_H
#define _MAPPING_H 1

#include <netinet/in.h>

struct host_mapping {
    struct host_mapping *next;
    struct host_mapping *prev;

    struct in_addr ip;
    char *name;
};

struct host_mapping *get_known_hosts();

#endif /* _MAPPING_H */
