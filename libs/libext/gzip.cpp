#include <ext/deflate.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

namespace Ext {

enum GZipFlags {
    FTEXT = 1,
    FHCRC = 2,
    FEXTRA = 4,
    FNAME = 8,
    FCOMMENT = 16,
};

Maybe<GZipData> read_gzip_path(const String& path) {
    int fd = open(path.string(), O_RDONLY);
    if (fd == -1) {
        perror("open");
        return {};
    }

    struct stat st;
    if (fstat(fd, &st)) {
        perror("fstat");
        close(fd);
        return {};
    }

    uint8_t* raw_data = (uint8_t*) mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (raw_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return {};
    }

    if (close(fd)) {
        perror("close");
        return {};
    }

    size_t offset = 0;
    if (raw_data[offset++] != 0x1F || raw_data[offset++] != 0x8B) {
        fprintf(stderr, "`%s' not gzip file\n", path.string());
        munmap(raw_data, st.st_size);
        return {};
    }

    uint8_t compression_method = raw_data[offset++];
    if (compression_method != 8) {
        fprintf(stderr, "`%s' has unsupported compression method: %u\n", path.string(), compression_method);
        munmap(raw_data, st.st_size);
        return {};
    }

    uint8_t flags = raw_data[offset++];
    time_t time_last_modified = *((uint32_t*) &raw_data[offset]);
    offset += sizeof(uint32_t);

    uint8_t extended_flags = raw_data[offset++];
    (void) extended_flags;

    uint8_t os = raw_data[offset++];
    (void) os;

    Maybe<Vector<uint8_t>> extra_data;
    if (flags & GZipFlags::FEXTRA) {
        uint16_t extra_length = *((uint16_t*) &raw_data[offset]);
        offset += sizeof(uint16_t);
        // FIXME: why doesn't this work?
        // extra_data = Vector<uint8_t>(&raw_data[offset], extra_length);
        offset += extra_length;
    }

    Maybe<String> original_name;
    if (flags & GZipFlags::FNAME) {
        original_name = String((char*) &raw_data[offset]);
        offset += original_name.value().size() + 1;
    }

    Maybe<String> comment;
    if (flags & GZipFlags::FCOMMENT) {
        comment = String((char*) &raw_data[offset]);
        offset += comment.value().size() + 1;
    }

    Maybe<uint16_t> crc16;
    if (flags & GZipFlags::FHCRC) {
        crc16 = *((uint16_t*) &raw_data[offset]);
        offset += sizeof(uint16_t);
    }

    uint32_t crc32 = *((uint32_t*) &raw_data[st.st_size - 2 * sizeof(uint32_t)]);
    (void) crc32;
    uint32_t isize = *((uint32_t*) &raw_data[st.st_size - sizeof(uint32_t)]);
    (void) isize;

    uint8_t* compressed_data_start = &raw_data[offset];
    size_t compressed_data_length = st.st_size - offset - 2 * sizeof(uint32_t);

    auto decompressed_data = decompress_deflate_payload(compressed_data_start, compressed_data_length);
    if (!decompressed_data.has_value()) {
        munmap(raw_data, st.st_size);
        return {};
    }

    munmap(raw_data, st.st_size);
    GZipData data;
    data.comment = move(comment);
    data.decompressed_data = move(decompressed_data.value());
    data.extra_data = move(extra_data);
    data.name = move(original_name);
    data.time_last_modified = time_last_modified;
    return move(data);
}

}
