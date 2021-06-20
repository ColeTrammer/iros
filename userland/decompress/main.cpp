#include <errno.h>
#include <ext/file.h>
#include <ext/gzip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    auto file = Ext::File::create(path, "r");
    if (!file) {
        fprintf(stderr, "decompress: failed to open file `%s': %s\n", path.string(), strerror(errno));
        return 1;
    }

    ByteBuffer output_buffer(BUFSIZ);
    Ext::GZipDecoder decoder;

    output_buffer.set_size(output_buffer.capacity());
    decoder.set_output(output_buffer.span());

    auto output_file = make_unique<Ext::File>(stdout);

    auto flush_output_buffer = [&] {
        output_buffer.set_size(decoder.writer().bytes_written());
        if (!output_file->write(output_buffer)) {
            fprintf(stderr, "compress: failed to write to file `%s': %s\n", "stdout", strerror(file->error()));
            exit(1);
        }
        decoder.did_flush_output();
        output_buffer.set_size(output_buffer.capacity());
    };

    auto stream_data = [&](const ByteBuffer& buffer) -> bool {
        auto result = decoder.stream_data(buffer.span());
        while (result == Ext::StreamResult::NeedsMoreOutputSpace) {
            flush_output_buffer();
            result = decoder.resume();
        }

        if (result == Ext::StreamResult::Error) {
            fprintf(stderr, "decompress: failed to decompress `%s'\n", path.string());
            exit(1);
        }

        flush_output_buffer();
        if (result == Ext::StreamResult::Success) {
            return false;
        }

        return true;
    };

    ByteBuffer input_buffer;
    if (!file->read_all_streamed(input_buffer, move(stream_data))) {
        fprintf(stderr, "decompress: failed to read file `%s': %s\n", path.string(), strerror(file->error()));
        return 1;
    }
    return 0;
}
