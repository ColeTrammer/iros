#include <ext/deflate.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s <path>\n", s);
    exit(1);
}

int main(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":")) != -1) {
        switch (opt) {
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (argc - optind != 1) {
        print_usage_and_exit(*argv);
    }

    auto data = Ext::read_gzip_path(argv[optind]);
    if (!data.has_value()) {
        fprintf(stderr, "decompress: failed to decode `%s'\n", argv[optind]);
        return 1;
    }

    fwrite(data.value().decompressed_data.vector(), 1, data.value().decompressed_data.size(), stdout);
    return 0;
}
