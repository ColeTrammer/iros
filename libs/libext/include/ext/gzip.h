#include <ext/deflate.h>

namespace Ext {
struct GZipData {
    Maybe<ByteBuffer> extra_data;
    Maybe<String> name;
    Maybe<String> comment;
    time_t time_last_modified;
};

class GZipDecoder {
public:
    GZipDecoder();
    ~GZipDecoder();

    const GZipData& member_data() const { return m_member_data; }

    void set_output(Span<uint8_t> output) { m_writer.set_output(output); }
    void did_flush_output() { m_writer.set_offset(0); }
    const ByteWriter& writer() const { return m_writer; }

    StreamResult stream_data(Span<const uint8_t> input);
    StreamResult resume();

private:
    Generator<StreamResult> decode();

    Generator<StreamResult> write_bytes(Span<const uint8_t> bytes);

    Generator<StreamResult> read_bytes(Span<uint8_t> bytes);
    Generator<StreamResult> read_string(String& string);

    Generator<StreamResult> m_decoder;
    ByteWriter m_writer;
    ByteReader m_reader;
    DeflateDecoder m_deflate_decoder;
    GZipData m_member_data;
};

class GZipEncoder {
public:
    GZipEncoder();
    ~GZipEncoder();

    void set_output(Span<uint8_t> output) { m_writer.set_output(output); }
    void did_flush_output() { m_writer.set_offset(0); }
    const ByteWriter& writer() const { return m_writer; }

    void set_name(String name) { m_gzip_data.name = move(name); }
    void set_comment(String comment) { m_gzip_data.comment = move(comment); }
    void set_time_last_modified(time_t time) { m_gzip_data.time_last_modified = time; }

    StreamResult stream_data(Span<const uint8_t> input, StreamFlushMode mode);
    StreamResult resume();

private:
    Generator<StreamResult> encode();

    Generator<StreamResult> write_bytes(Span<const uint8_t> bytes);

    Generator<StreamResult> m_encoder;
    StreamFlushMode m_flush_mode { StreamFlushMode::NoFlush };
    Span<const uint8_t> m_input;
    ByteWriter m_writer;
    DeflateEncoder m_deflate_encoder;
    GZipData m_gzip_data;
};
}
