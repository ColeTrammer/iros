#include <errno.h>
#include <ext/deflate.h>
#include <ext/mapped_file.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
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

    String path = argv[1];

    ByteWriter output_writer(0x4000);
    Ext::GZipEncoder encoder(output_writer);

    encoder.set_name(path);
    encoder.set_time_last_modified(0);

    auto file = Ext::MappedFile::create(path, PROT_READ, MAP_SHARED);
    if (!file) {
        fprintf(stderr, "%s: failed to open file `%s': %s", *argv, path.string(), strerror(errno));
        return 1;
    }

    auto result = encoder.stream_data({ file->data(), file->size() }, Ext::FlushMode::StreamFlush);
    if (result != Ext::StreamResult::Success) {
        fprintf(stderr, "%s: failed to compress file `%s'", *argv, path.string());
        return 1;
    }

    fwrite(output_writer.data(), 1, output_writer.buffer_size(), stdout);

    return 0;
}
