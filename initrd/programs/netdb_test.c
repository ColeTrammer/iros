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

void test_networks(void) {
    struct netent *net;
    setnetent(1);
    while ((net = getnetent())) {
        printf("[network]: '%s' |%d| (", net->n_name, net->n_addrtype);
        for (size_t i = 0; net->n_aliases[i]; i++) {
            printf("%s,", net->n_aliases[i]);
        }
        struct in_addr addr = { .s_addr = htonl(net->n_net) };
        printf(") => '%s'\n", inet_ntoa(addr));
    }

    assert(getnetbyaddr(ntohl(inet_addr("169.254.0.0")), AF_INET));
    assert(getnetbyname("link-local"));

    endnetent();
}

void test_services(void) {
    struct servent *service;
    setservent(1);
    while ((service = getservent())) {
        printf("[service]: '%s' |%d| {'%s'} (", service->s_name, ntohl(service->s_port), service->s_proto);
        for (size_t i = 0; service->s_aliases[i]; i++) {
            printf("%s,", service->s_aliases[i]);
        }
        printf(")\n");
    }

    assert(getservbyname("domain", "udp"));
    assert(getservbyname("http", "tcp"));
    assert(getservbyname("www", NULL));
    assert(getservbyname("https", "tcp"));

    assert(getservbyport(htonl(53), "udp"));
    assert(getservbyport(htonl(80), NULL));
    assert(getservbyport(htonl(443), "tcp"));

    endservent();
}

static void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-hnps]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    bool all = true;
    bool test_h = false;
    bool test_n = false;
    bool test_p = false;
    bool test_s = false;

    int opt;
    while ((opt = getopt(argc, argv, ":hnps")) != -1) {
        switch (opt) {
            case 'h':
                test_h = true;
                all = false;
                break;
            case 'n':
                test_n = true;
                all = false;
                break;
            case 'p':
                test_p = true;
                all = false;
                break;
            case 's':
                test_s = true;
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

    if (all || test_n) {
        test_networks();
    }

    if (all || test_p) {
        test_protocols();
    }

    if (all || test_s) {
        test_services();
    }

    return any_failed;
}
