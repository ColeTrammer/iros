#ifndef _DNS_SERVICE_MESSAGE_H
#define _DNS_SERVICE_MESSAGE_H 1

#include <stdint.h>

enum dns_request_type {
    DNS_REQUEST_LOOKUP,
    DNS_REQUEST_REVERSE,
};

struct dns_request {
    enum dns_request_type type;
    uint8_t request[0];
};

struct dns_response {
    uint8_t success;
    uint8_t response[0];
};

#endif /* _DNS_SERVICE_MESSAGE_H */
