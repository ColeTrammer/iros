#include <ext/deflate.h>

namespace Ext {

Maybe<Vector<uint8_t>> decompress_deflate_payload(uint8_t* compressed_data, size_t compressed_data_length) {
    (void) compressed_data;
    (void) compressed_data_length;

    return {};
}

}
