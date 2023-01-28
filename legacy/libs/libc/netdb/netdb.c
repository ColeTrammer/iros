#include <arpa/inet.h>
#include <dns_service/message.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int getaddrinfo(const char *__restrict node, const char *__restrict service, const struct addrinfo *__restrict hints,
                struct addrinfo **__restrict res) {
    (void) service;
    (void) hints;

    struct addrinfo *result = calloc(1, sizeof(struct addrinfo));
    *res = result;

    struct sockaddr_in *found = calloc(1, sizeof(struct sockaddr_in));
    found->sin_family = AF_INET;
    found->sin_port = 0;
    result->ai_addr = (struct sockaddr *) found;
    result->ai_addrlen = sizeof(struct sockaddr_in);

    if (node == NULL) {
        found->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        return 0;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        free(result);
        free(found);
        return EAI_SYSTEM;
    }

    struct sockaddr_un conn = { 0 };
    conn.sun_family = AF_UNIX;
    strcpy(conn.sun_path, "/tmp/.nslookup.socket");

    if (connect(fd, (const struct sockaddr *) &conn, sizeof(struct sockaddr_un)) == -1) {
        free(result);
        free(found);
        close(fd);
        return EAI_SYSTEM;
    }

    size_t name_length = strlen(node) + 1;
    if (name_length > 1024) {
        free(result);
        free(found);
        close(fd);
        return EAI_NONAME;
    }

    char send_buffer[sizeof(struct dns_request) + name_length + 1];
    struct dns_request *request = (struct dns_request *) send_buffer;
    request->type = DNS_REQUEST_LOOKUP;
    memcpy(request->request, node, name_length);

    if (write(fd, send_buffer, sizeof(send_buffer)) == -1) {
        free(result);
        free(found);
        close(fd);
        return EAI_SYSTEM;
    }

    char buf[32];
    if (read(fd, buf, 32) <= 0) {
        free(result);
        free(found);
        close(fd);
        return EAI_SYSTEM;
    }

    struct dns_response *response = (struct dns_response *) buf;
    if (!response->success) {
        free(result);
        free(found);
        close(fd);
        return EAI_NONAME;
    }

    found->sin_addr.s_addr = *((in_addr_t *) response->response);

    close(fd);
    return 0;
}

void freeaddrinfo(struct addrinfo *res) {
    while (res != NULL) {
        struct addrinfo *next = res->ai_next;
        free(res->ai_addr);
        free(res);
        res = next;
    }
}

int getnameinfo(const struct sockaddr *__restrict addr, socklen_t addrlen, char *__restrict host, socklen_t hostlen, char *__restrict serv,
                socklen_t servlen, int flags) {
    (void) flags;
    (void) host;
    (void) hostlen;
    (void) serv;
    (void) servlen;

    if (addr->sa_family != AF_INET || addrlen < sizeof(struct sockaddr_in)) {
        return EAI_FAMILY;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        return EAI_SYSTEM;
    }

    return EAI_NONAME;
}

const char *gai_strerror(int err) {
    switch (err) {
        case EAI_NONAME:
            return "No name";
        case EAI_FAMILY:
            return "Bad address family";
        case EAI_SYSTEM:
            return "System error";
        case EAI_SERVICE:
            return "Service error";
        default:
            return "Invalid error";
    }
}
