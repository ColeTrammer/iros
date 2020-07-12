#pragma once

#include <liim/maybe.h>
#include <liim/string.h>
#include <liim/vector.h>
#include <stdint.h>
#include <sys/types.h>

namespace Ext {

Maybe<Vector<uint8_t>> decompress_deflate_payload(uint8_t* compressed_data, size_t compressed_data_length);

struct GZipData {
    Maybe<Vector<uint8_t>> extra_data;
    Maybe<String> name;
    Maybe<String> comment;
    time_t time_last_modified;
    Vector<uint8_t> decompressed_data;
};

Maybe<GZipData> read_gzip_path(const String& path);

}
