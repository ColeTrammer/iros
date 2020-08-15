#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int any_failed = 0;

static void test_hosts(void) {
    struct hostent *host;
    sethostent(1);
    while ((host = gethostent())) {
        printf("[host]: '%s' |%d, %d| (", host->h_name, host->h_addrtype, host->h_length);
        char *iter;
        size_t i = 0;
        while ((iter = host->h_aliases[i++])) {
            printf("%s,", iter);
        }
        printf(") => {");
        i = 0;
        while ((iter = host->h_addr_list[i++])) {
            printf("%s,", inet_ntoa(*(struct in_addr *) iter));
        }
        printf("}\n");
    }
    endhostent();

    host = gethostbyname("google.com");
    assert(host);
    printf("[host]: '%s' |%d, %d| (", host->h_name, host->h_addrtype, host->h_length);
    for (size_t i = 0; host->h_aliases[i]; i++) {
        printf("%s,", host->h_aliases[i]);
    }
    printf(") => {");
    for (size_t i = 0; host->h_addr_list[i]; i++) {
        printf("%s,", inet_ntoa(*(struct in_addr *) host->h_addr_list[i]));
    }
    printf("}\n");
}

static void test_protocols(void) {
    struct protoent *proto;
    setprotoent(1);
    while ((proto = getprotoent())) {
        printf("[protocol]: '%s' |%d| (", proto->p_name, proto->p_proto);
        char *iter;
        size_t i = 0;
        while ((iter = proto->p_aliases[i++])) {
            printf("%s,", iter);
        }
        printf(")\n");
    }

    assert(getprotobynumber(IPPROTO_ICMP));
    assert(getprotobynumber(IPPROTO_IP));
    assert(getprotobynumber(IPPROTO_TCP));
    assert(getprotobynumber(IPPROTO_UDP));
    assert(getprotobynumber(IPPROTO_IPV6));

    assert(getprotobyname("icmp"));
    assert(getprotobyname("ip"));
    assert(getprotobyname("ipv6"));
    assert(getprotobyname("tcp"));
    assert(getprotobyname("udp"));

    endprotoent();
}

static void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-hp]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    bool all = true;
    bool test_h = false;
    bool test_p = false;

    int opt;
    while ((opt = getopt(argc, argv, ":hp")) != -1) {
        switch (opt) {
            case 'h':
                test_h = true;
                all = false;
                break;
            case 'p':
                test_p = true;
                all = false;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (optind != argc) {
        print_usage_and_exit(*argv);
    }

    if (all || test_h) {
        test_hosts();
    }

    if (all || test_p) {
        test_protocols();
    }

    return any_failed;
}
