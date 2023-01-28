#include <arpa/inet.h>
#include <stdio.h>

static char ntoa_buf[INET_ADDRSTRLEN] = { 0 };

char *inet_ntoa(struct in_addr in) {
    snprintf(ntoa_buf, 16, "%.3d.%.3d.%.3d.%.3d", (in.s_addr & 0x000000FF) >> 0, (in.s_addr & 0x0000FF00) >> 8,
             (in.s_addr & 0x00FF0000) >> 16, (in.s_addr & 0xFF000000) >> 24);
    return ntoa_buf;
}
