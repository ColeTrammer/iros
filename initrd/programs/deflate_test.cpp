#include <ext/deflate.h>
#include <unistd.h>

int main() {
    uint8_t data[27];
    strcpy((char*) data, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    Ext::DeflateEncoder encoder;
    ByteBuffer output_buffer(0x10000);
    output_buffer.set_size(output_buffer.capacity());
    encoder.set_output(output_buffer.span());

    auto result = encoder.stream_data({ data, sizeof(data) }, Ext::StreamFlushMode::StreamFlush);
    if (result == Ext::StreamResult::Error) {
        fprintf(stderr, "Error encoding data\n");
        return 1;
    }

    printf("Encoded: '%s'\n", (char*) data);
    output_buffer.set_size(encoder.writer().bytes_written());
    assert(result != Ext::StreamResult::NeedsMoreInput);

    Ext::DeflateDecoder decoder;
    ByteBuffer new_buffer(0x10000);
    new_buffer.set_size(new_buffer.capacity());
    decoder.set_output(new_buffer.span());
    result = decoder.stream_data(output_buffer.span());
    assert(result == Ext::StreamResult::Success);
    auto string = String((char*) new_buffer.data());
    printf("Decoded: '%s'\n", string.string());

    return 0;
}
