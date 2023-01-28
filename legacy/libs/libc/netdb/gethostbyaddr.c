#include <arpa/inet.h>
#include <bits/field_parser.h>
#include <dns_service/message.h>
#include <netdb.h>
#include <stddef.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

static struct hostent __static_hostbyaddr_hostent;
static char __static_hostbyaddr_buffer[1024];

static int __fill_hostent(const char *name, in_addr_t addr) {
    size_t buffer_index = 0;

    __static_hostbyaddr_hostent.h_addrtype = AF_INET;
    __static_hostbyaddr_hostent.h_length = sizeof(in_addr_t);
    __static_hostbyaddr_hostent.h_name =
        TRY_WRITE_STRING(name, __static_hostbyaddr_buffer, buffer_index, sizeof(__static_hostbyaddr_buffer));
    __static_hostbyaddr_hostent.h_aliases =
        TRY_WRITE_BUF(NULL, __static_hostbyaddr_buffer, buffer_index, sizeof(__static_hostbyaddr_buffer));
    char *addr_p = TRY_WRITE_BUF(addr, __static_hostbyaddr_buffer, buffer_index, sizeof(__static_hostbyaddr_buffer));
    __static_hostbyaddr_hostent.h_addr_list =
        TRY_WRITE_BUF(addr_p, __static_hostbyaddr_buffer, buffer_index, sizeof(__static_hostbyaddr_buffer));
    TRY_WRITE_BUF(NULL, __static_hostbyaddr_buffer, buffer_index, sizeof(__static_hostbyaddr_buffer));
    return 0;
}

struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type) {
    if (len != sizeof(struct in_addr) && type != AF_INET) {
        h_errno = HOST_NOT_FOUND;
        return NULL;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        h_errno = NO_RECOVERY;
        return NULL;
    }

    struct sockaddr_un conn = { 0 };
    conn.sun_family = AF_UNIX;
    strcpy(conn.sun_path, "/tmp/.nslookup.socket");

    if (connect(fd, (const struct sockaddr *) &conn, sizeof(struct sockaddr_un)) == -1) {
        close(fd);
        h_errno = NO_RECOVERY;
        return NULL;
    }

    char send_buffer[sizeof(struct dns_request) + len];
    struct dns_request *request = (struct dns_request *) send_buffer;
    request->type = DNS_REQUEST_REVERSE;
    memcpy(request->request, addr, len);

    if (write(fd, send_buffer, sizeof(send_buffer)) == -1) {
        close(fd);
        h_errno = NO_RECOVERY;
        return NULL;
    }

    char buf[sizeof(struct dns_response) + 1024];
    if (read(fd, buf, 32) <= 0) {
        close(fd);
        h_errno = NO_RECOVERY;
        return NULL;
    }

    struct dns_response *response = (struct dns_response *) buf;
    if (!response->success) {
        close(fd);
        h_errno = HOST_NOT_FOUND;
        return NULL;
    }
    const char *name = (const char *) response->response;

    close(fd);

    int ret = __fill_hostent(name, *(in_addr_t *) addr);
    if (ret) {
        h_errno = NO_RECOVERY;
        return NULL;
    }

    return &__static_hostbyaddr_hostent;
}
