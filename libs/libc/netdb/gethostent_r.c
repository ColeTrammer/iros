#include <arpa/inet.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

FILE *__hostent_file = NULL;

#define TRY_WRITE_BUF(data, buffer, i, max)      \
    ({                                           \
        if (i + sizeof(data) > max) {            \
            *h_errnop = NO_DATA;                 \
            return -1;                           \
        }                                        \
        *(__typeof__(data) *) &buffer[i] = data; \
        i += sizeof(data);                       \
        (void *) &buffer[i - sizeof(data)];      \
    })

#define TRY_WRITE_STRING(s, buffer, i, max) \
    ({                                      \
        size_t __slen = strlen(s) + 1;      \
        if (i + __slen > max) {             \
            *h_errnop = NO_DATA;            \
            return -1;                      \
        }                                   \
        memcpy(&buffer[i], s, __slen);      \
        i += __slen;                        \
        (void *) &buffer[i - __slen];       \
    })

int gethostent_r(struct hostent *ret, char *buf, size_t buflen, struct hostent **result, int *h_errnop) {
    if (!__hostent_file) {
        __hostent_file = fopen("/etc/hosts", "w");
        if (!__hostent_file) {
            *h_errnop = NO_DATA;
            return -1;
        }
    }

    char line_buffer[_POSIX2_LINE_MAX];
    char *line = fgets(line_buffer, sizeof(line_buffer), __hostent_file);
    if (!line) {
        *h_errnop = NO_DATA;
        return -1;
    }

    size_t field_index = 0;
    size_t buf_index = 0;
    char **aliases = NULL;

    char *save;
    char *field = strtok_r(line, " ", &save);
    while (field) {
        switch (field_index) {
            case 0: {
                in_addr_t addr = inet_addr(field);
                if (addr == INADDR_NONE) {
                    *h_errnop = NO_DATA;
                    return -1;
                }
                char *addr_p = TRY_WRITE_BUF(addr, buf, buf_index, buflen);
                char **addresses = TRY_WRITE_BUF(addr_p, buf, buf_index, buflen);
                TRY_WRITE_BUF(NULL, buf, buf_index, buflen);
                ret->h_addr_list = addresses;
                ret->h_addrtype = AF_INET;
                ret->h_length = sizeof(in_addr_t);
                break;
            }
            case 1:
                ret->h_name = TRY_WRITE_STRING(line, buf, buf_index, buflen);
                break;
            case 2:
                break;
            default:
                break;
        }

        field = strtok_r(NULL, " ", &save);
        field_index++;
    }

    if (field_index < 2) {
        *h_errnop = NO_DATA;
        return -1;
    }

    if (!aliases) {
        ret->h_aliases = TRY_WRITE_BUF(NULL, buf, buf_index, buflen);
    }

    *result = ret;
    return 0;
}
