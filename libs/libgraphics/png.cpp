#include <arpa/inet.h>
#include <ext/deflate.h>
#include <ext/mapped_file.h>
#include <graphics/png.h>

#define PNG_DEBUG

SharedPtr<PixelBuffer> decode_png_image(uint8_t* data, size_t size) {
    LIIM::size_t offset = 0;
    bool seen_ihdr = false;
    bool successfully_decoded_idat = false;
    bool seen_iend = false;
    int width = 0;
    int height = 0;
    int bit_depth = 0;
    int color_type = 0;
    Ext::ZLibStreamDecoder idat_decoder;

    auto get = [&](size_t bytes) -> Maybe<uint64_t> {
        if (offset + bytes > size) {
            return {};
        }

        uint64_t value = 0;
        for (size_t i = 0; i < bytes; i++) {
            // NOTE: this reads the number in network byte order.
            uint64_t byte = data[offset++];
            auto shift = ((bytes - i - 1UL) * 8UL);
            value |= (byte << shift);
        }
        return value;
    };

    auto decode_ihdr = [&](size_t chunk_length) -> bool {
        if (chunk_length != 13) {
            return false;
        }

        if (offset + 13 > size) {
            return false;
        }

        struct IHDR {
            uint32_t width;
            uint32_t height;
            uint8_t bit_depth;
            uint8_t color_type;
            uint8_t compression_method;
            uint8_t filter_method;
            uint8_t interlace_method;
        } __attribute__((packed));

        IHDR* header = reinterpret_cast<IHDR*>(&data[offset]);
        width = ntohl(header->width);
        height = ntohl(header->height);
        bit_depth = header->bit_depth;
        color_type = header->color_type;

#ifdef PNG_DEBUG
        fprintf(stderr, "width=%d height=%d bit_depth=%d color_type=%d\n", width, height, bit_depth, color_type);
#endif /* PNG_DEBUG */

        if (header->compression_method != 0) {
            fprintf(stderr, "Unsupported PNG compression method: %u\n", header->compression_method);
            return false;
        }

        if (header->filter_method != 0) {
            fprintf(stderr, "Unsupported PNG filter method: %u\n", header->filter_method);
            return false;
        }

        if (header->interlace_method != 0) {
            fprintf(stderr, "Unsupported PNG interlace method; %u\n", header->interlace_method);
            return false;
        }

        offset += chunk_length;
        seen_ihdr = true;
        return true;
    };

    auto decode_idat = [&](size_t chunk_length) -> bool {
        if (offset + chunk_length > size) {
            return false;
        }

        auto result = idat_decoder.stream_data(&data[offset], chunk_length);
        if (result == Ext::StreamResult::Error) {
            return false;
        } else if (result == Ext::StreamResult::Success) {
            successfully_decoded_idat = true;
        }

        offset += chunk_length;
        return true;
    };

    auto header = get(8);
    if (!header.has_value()) {
        return nullptr;
    }

    if (header.value() != 0x89'50'4E'47'0D'0A'1A'0A) {
        return nullptr;
    }

    while (!seen_iend) {
        auto chunk_length = get(4);
        if (!chunk_length.has_value()) {
            return nullptr;
        }

        auto chunk_type = get(4);
        if (!chunk_type.has_value()) {
            return nullptr;
        }

        if (chunk_type.value() == (('I' << 24) | ('H' << 16) | ('D' << 8) | 'R')) {
            if (!decode_ihdr(chunk_length.value())) {
                return nullptr;
            }
        } else if (chunk_type.value() == (('I' << 24) | ('D' << 16) | ('A' << 8) | 'T')) {
            if (!decode_idat(chunk_length.value())) {
                return nullptr;
            }
        } else if (chunk_type.value() == (('I' << 24) | ('E' << 16) | ('N' << 8) | 'D')) {
            if (!seen_ihdr || !successfully_decoded_idat) {
                return nullptr;
            }
            if (chunk_length.value() != 0) {
                return nullptr;
            }
            seen_iend = true;
        } else {
            String type;
            type.insert(static_cast<char>(chunk_type.value() >> 24), type.size());
            type.insert(static_cast<char>(chunk_type.value() >> 16), type.size());
            type.insert(static_cast<char>(chunk_type.value() >> 8), type.size());
            type.insert(static_cast<char>(chunk_type.value()), type.size());
#ifdef PNG_DEBUG
            fprintf(stderr, "Unkown chunk type '%s'\n", type.string());
#endif /* PNG_DEBUG */
            offset += chunk_length.value();
        }

        auto crc = get(4);
        if (!crc.has_value()) {
            return nullptr;
        }
    }

    auto bitmap = make_shared<PixelBuffer>(width, height);
    bitmap->clear(ColorValue::White);
    return bitmap;
}

SharedPtr<PixelBuffer> decode_png_file(const String& path) {
    auto mapped_file = Ext::MappedFile::create(path);
    if (!mapped_file) {
        return nullptr;
    }

    return decode_png_image(mapped_file->data(), mapped_file->size());
}
