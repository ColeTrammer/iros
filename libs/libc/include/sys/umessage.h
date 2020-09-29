#ifndef _SYS_UMESSAGE_H
#define _SYS_UMESSAGE_H 1

#include <net/if.h>
#include <stdint.h>

enum umessage_category_number {
    UMESSAGE_INTERFACE,
    UMESSAGE_NUM_CATEGORIES,
};

struct umessage {
    uint32_t length;
    uint16_t category;
    uint16_t type;
    char data[0];
};
#define UMESSAGE_VALID(u, len) ((len) >= sizeof(struct umessage) && (u)->length == (len) && (u)->category < UMESSAGE_NUM_CATEGORIES)

enum umessage_interface_request_type {
    UMESSAGE_INTERFACE_LIST_REQUEST,
    UMESSAGE_INTERFACE_NUM_REQUESTS,
};
#define UMESSAGE_INTERFACE_REQUEST_VALID(u, len) ((u)->category == UMESSAGE_INTERFACE && (u)->type < UMESSAGE_INTERFACE_NUM_REQUESTS)

enum umessage_interface_message_type {
    UMESSAGE_INTERFACE_LIST,
    UMESSAGE_INTERFACE_NUM_MESSAGES,
};
#define UMESSAGE_INTERFACE_MESSAGE_VALID(u, len) \
    (UMESSAGE_VALID(u, len) && (u)->category == UMESSAGE_INTERFACE && (u)->type < UMESSAGE_INTERFACE_NUM_MESSAGES)

struct umessage_interface_list_request {
    struct umessage base;
};
#define UMESSAFE_INTERFACE_LIST_REQUEST_VALID(u, len)                                                       \
    (UMESSAGE_INTERFACE_REQUEST_VALID(u, len) && (len) >= sizeof(struct umessage_interface_list_request) && \
     (u)->type == UMESSAGE_INTERFACE_LIST_REQUEST)

struct umessage_interface_desc {
    char name[IF_NAMESIZE];
    int index;
};

struct umessage_interface_list {
    struct umessage base;
    size_t interface_count;
    struct umessage_interface_desc interface_list[0];
};
#define UMESSAGE_INTERFACE_LIST_LENGTH(icount) (sizeof(struct umessage_interface_list) + (icount) * sizeof(struct umessage_interface_desc))
#define UMESSAGE_INTERFACE_LIST_COUNT(length)            \
    (((length) < sizeof(struct umessage_interface_list)) \
         ? 0                                             \
         : ((length) - sizeof(struct umessage_interface_list)) / sizeof(struct umessage_interface_desc))
#define UMESSAGE_INTERFACE_LIST_VALID(u, len)                                                       \
    (UMESSAGE_INTERFACE_MESSAGE_VALID(u, len) && (len) >= sizeof(struct umessage_interface_list) && \
     (u)->type == UMESSAGE_INTERFACE_LIST &&                                                        \
     ((struct umessage_interface_list*) (u))->interface_count == UMESSAGE_INTERFACE_LIST_COUNT(len))

#endif /* _SYS_UMESSAGE_H */
