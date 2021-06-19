#include <ext/deflate.h>

namespace Ext {
struct GZipData {
    Maybe<Vector<uint8_t>> extra_data;
    Maybe<String> name;
    Maybe<String> comment;
    time_t time_last_modified;
    Vector<uint8_t> decompressed_data;
};

class GZipEncoder {
public:
    GZipEncoder(ByteWriter& writer);
    ~GZipEncoder();

    void set_name(String name) { m_gzip_data.name = move(name); }
    void set_comment(String comment) { m_gzip_data.comment = move(comment); }
    void set_time_last_modified(time_t time) { m_gzip_data.time_last_modified = time; }

    StreamResult stream_data(Span<const uint8_t> input, StreamFlushMode mode);

private:
    Generator<StreamResult> encode();

    Generator<StreamResult> write_bytes(const uint8_t* bytes, size_t byte_count);

    Generator<StreamResult> m_encoder;
    StreamFlushMode m_flush_mode { StreamFlushMode::NoFlush };
    Span<const uint8_t> m_input;
    ByteWriter& m_writer;
    DeflateEncoder m_deflate_encoder;
    GZipData m_gzip_data;
};

Maybe<GZipData> read_gzip_path(const String& path);
}
