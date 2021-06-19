#include <errno.h>
#include <ext/file.h>
#include <ext/gzip.h>
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

    String path = argv[optind];

    ByteWriter writer;
    Ext::GZipEncoder encoder(writer);

    encoder.set_name(path);
    encoder.set_time_last_modified(0);

    auto file = Ext::File::create(path, "r");
    if (!file) {
        fprintf(stderr, "compress: failed to open file `%s': %s\n", path.string(), strerror(errno));
        return 1;
    }

    auto stream_data = [&](const ByteBuffer& buffer) -> bool {
        auto flush_mode = buffer.empty() ? Ext::StreamFlushMode::StreamFlush : Ext::StreamFlushMode::NoFlush;
        auto result = encoder.stream_data(buffer.span(), flush_mode);
        if (result == Ext::StreamResult::Error || result == Ext::StreamResult::NeedsMoreOutputSpace) {
            fprintf(stderr, "compress: failed to compress `%s'\n", path.string());
            exit(1);
        }

        return true;
    };

    ByteBuffer input_buffer;
    if (!file->read_all_streamed(input_buffer, move(stream_data))) {
        fprintf(stderr, "compress: failed to read file `%s': %s\n", path.string(), strerror(errno));
        return 1;
    }

    fwrite(writer.data(), 1, writer.buffer_size(), stdout);
    return 0;
}
