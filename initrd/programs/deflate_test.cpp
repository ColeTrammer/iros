#include <ext/deflate.h>
#include <unistd.h>

int main() {
    uint8_t data[27];
    strcpy((char*) data, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    Vector<uint8_t> output_buffer;
    output_buffer.resize(0x2000);

    Ext::DeflateEncoder encoder;
    encoder.set_output_buffer(output_buffer.vector(), output_buffer.size());
    auto result = encoder.stream_data(data, sizeof(data));
    if (result == Ext::StreamResult::Error) {
        fprintf(stderr, "Error encoding data\n");
        return 1;
    }

    printf("Encoded: '%s'\n", (char*) data);
    assert(result != Ext::StreamResult::NeedsMoreData);

    Ext::DeflateDecoder decoder;
    result = decoder.stream_data(output_buffer.vector(), output_buffer.size());
    assert(result == Ext::StreamResult::Success);
    auto string = String((char*) decoder.decompressed_data().vector());
    printf("Decoded: '%s'\n", string.string());

    return 0;
}
