#include <ext/mapped_file.h>
#include <graphics/png.h>

SharedPtr<PixelBuffer> decode_png_image(uint8_t* data, size_t size) {
    size_t offset = 0;

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

    auto header = get(8);
    if (!header.has_value()) {
        return nullptr;
    }

    if (header.value() != 0x89'50'4E'47'0D'0A'1A'0A) {
        return nullptr;
    }

    return nullptr;
}

SharedPtr<PixelBuffer> decode_png_file(const String& path) {
    auto mapped_file = Ext::MappedFile::create(path);
    if (!mapped_file) {
        return nullptr;
    }

    return decode_png_image(mapped_file->data(), mapped_file->size());
}
