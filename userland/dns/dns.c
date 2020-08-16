#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include "dns.h"
#include "mapping.h"

#define perror(s) syslog(LOG_ERR, s ": %m")

static ssize_t send_dns(char *host, char *buf, size_t buf_max, size_t *new_len, int q_type) {
    size_t old_len = strlen(host);
    *new_len = sizeof(struct dns_header) + sizeof(struct dns_question) + 1;

    char *part = strtok(host, ".");
    while (part != NULL) {
        *new_len += strlen(part) + 1;
        part = strtok(NULL, ".");
    }

    uint8_t *message = calloc(*new_len, sizeof(char));
    struct dns_header *header = (struct dns_header *) message;
    header->id = ntohs(getpid());
    header->qr = 0;
    header->op_code = 0;
    header->autoratative = 0;
    header->truncated = 0;
    header->recursion_desired = 1;
    header->recursion_available = 0;
    header->zero = 0;
    header->response_code = 0;
    header->num_questions = ntohs(1);
    header->num_answers = 0;
    header->num_records = 0;
    header->num_records_extra = 0;

    size_t name_offset = 0;
    for (size_t i = 0; i < old_len; i++) {
        char *part = host + i;
        size_t part_len = strlen(part);
        message[sizeof(struct dns_header) + name_offset++] = (uint8_t) part_len;
        name_offset += sprintf((char *) (message + name_offset + sizeof(struct dns_header)), "%s", part);
        i += part_len;
    }

    message[*new_len - sizeof(struct dns_question) - 1] = '\0';

    struct dns_question *question = (struct dns_question *) (message + *new_len - sizeof(struct dns_question));
    question->type = htons(q_type);
    question->class = htons(DNS_CLASS_INET);

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == -1) {
        free(message);
        perror("socket");
        return -1;
    }

    struct timeval tv = { 1, 0 };
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) == -1) {
        perror("setsockopt");
        free(message);
        return -1;
    }

    struct sockaddr_in dest = { 0 };
    dest.sin_family = AF_INET;
    dest.sin_port = htons(DNS_PORT);
    dest.sin_addr.s_addr = inet_addr(DNS_IP);

    if (sendto(fd, message, *new_len, 0, (const struct sockaddr *) &dest, sizeof(struct sockaddr_in)) == -1) {
        perror("sendto");
        free(message);
        return -1;
    }

    free(message);

    struct sockaddr_in source = { 0 };
    socklen_t source_len = sizeof(struct sockaddr_in);

    ssize_t read = -1;

    for (int i = 0; read == -1 && i < 3; i++) {
        read = recvfrom(fd, buf, buf_max, 0, (struct sockaddr *) &source, &source_len);
    }

    close(fd);

    if (read < (ssize_t) sizeof(struct dns_header)) {
        return -1;
    }

    return read;
}

struct host_mapping *lookup_host(char *host) {
    char *host_save = strdup(host);

    char buf[1024];
    size_t new_len;
    ssize_t ret = send_dns(host, buf, sizeof(buf), &new_len, DNS_TYPE_A);
    if (ret < 0) {
        free(host_save);
        return NULL;
    }

    struct dns_header *response_header = (struct dns_header *) buf;
    assert(response_header->qr == 1);

    syslog(LOG_INFO, "Recieved response: %u, %u, %u, %u, %u", ntohs(response_header->id), ntohs(response_header->num_questions),
           ntohs(response_header->num_answers), ntohs(response_header->num_records), ntohs(response_header->num_records_extra));

    struct dns_record *record = (struct dns_record *) (buf + new_len);
    syslog(LOG_INFO, "Received record: %u, %u, %u, %u", ntohs(record->type), ntohs(record->class), ntohl(record->ttl),
           ntohs(record->rd_length));

    struct in_addr res = { 0 };
    res.s_addr = *((uint32_t *) (record + 1));

    struct host_mapping *mapping = calloc(1, sizeof(struct host_mapping));
    mapping->ip = res;
    mapping->name = host_save;

    return mapping;
}

struct host_mapping *lookup_address(uint32_t ip_address) {
    char host[255];
    snprintf(host, sizeof(host) - 1, "%d.%d.%d.%d.in-addr.arpa", (ip_address & 0x000000FFU) >> 0U, (ip_address & 0x0000FF00U) >> 8U,
             (ip_address & 0x00FF0000U) >> 16U, (ip_address & 0xFF000000U) >> 24U);

    char buf[1024];
    size_t new_len;
    ssize_t ret = send_dns(host, buf, sizeof(buf), &new_len, DNS_TYPE_PTR);
    if (ret < 0) {
        return NULL;
    }

    struct dns_header *response_header = (struct dns_header *) buf;
    assert(response_header->qr == 1);

    syslog(LOG_INFO, "Recieved response: %u, %u, %u, %u, %u", ntohs(response_header->id), ntohs(response_header->num_questions),
           ntohs(response_header->num_answers), ntohs(response_header->num_records), ntohs(response_header->num_records_extra));

    struct dns_record *record = (struct dns_record *) (buf + new_len);
    syslog(LOG_INFO, "Received record: %u, %u, %u, %u", ntohs(record->type), ntohs(record->class), ntohl(record->ttl),
           ntohs(record->rd_length));

    char *raw_domain_name = ((char *) (record + 1));
    for (size_t i = 0; raw_domain_name[i] && i < ntohs(record->rd_length); i++) {
        uint8_t len = raw_domain_name[i];
        raw_domain_name[i] = '.';
        i += len;
    }

    struct host_mapping *mapping = calloc(1, sizeof(struct host_mapping));
    mapping->ip.s_addr = htonl(ip_address);
    mapping->name = strdup(raw_domain_name + 1);

    return mapping;
}
