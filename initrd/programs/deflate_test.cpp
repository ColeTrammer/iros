#include <ext/deflate.h>
#include <unistd.h>

int main() {
    uint8_t data[27];
    strcpy((char*) data, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    ByteWriter writer;
    Ext::DeflateEncoder encoder(writer);
    auto result = encoder.stream_data({ data, sizeof(data) }, Ext::FlushMode::StreamFlush);
    if (result == Ext::StreamResult::Error) {
        fprintf(stderr, "Error encoding data\n");
        return 1;
    }

    printf("Encoded: '%s'\n", (char*) data);
    assert(result != Ext::StreamResult::NeedsMoreData);

    Ext::DeflateDecoder decoder;
    result = decoder.stream_data(writer.data(), writer.buffer_size());
    assert(result == Ext::StreamResult::Success);
    auto string = String((char*) decoder.decompressed_data().vector());
    printf("Decoded: '%s'\n", string.string());

    return 0;
}
