#ifndef _DNS_H
#define _DNS_H 1

#define DNS_IP   "8.8.8.8"
#define DNS_PORT 53

#define DNS_TYPE_A   1  // Host address
#define DNS_TYPE_PTR 12 // Host name pointer

#define DNS_CLASS_INET 1 // Internet

struct host_mapping;

struct dns_header {
    uint16_t id;

    uint8_t recursion_desired : 1;
    uint8_t truncated : 1;
    uint8_t autoratative : 1;
    uint8_t op_code : 4;
    uint8_t qr : 1;
    uint8_t response_code : 4;
    uint8_t zero : 3;
    uint8_t recursion_available : 1;

    uint16_t num_questions;
    uint16_t num_answers;
    uint16_t num_records;
    uint16_t num_records_extra;
} __attribute__((packed));

struct dns_question {
    uint16_t type;
    uint16_t class;
} __attribute__((packed));

struct dns_record {
    uint16_t name;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rd_length;
} __attribute__((packed));

struct host_mapping *lookup_host(char *host);
struct host_mapping *lookup_address(uint32_t ip_address);

#endif /* _DNS_H */
