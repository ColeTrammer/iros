#include <ext/deflate.h>

namespace Ext {
struct GZipData {
    Option<ByteBuffer> extra_data;
    Option<String> name;
    Option<String> comment;
    time_t time_last_modified;
};

class GZipDecoder final : public StreamDecoder {
public:
    GZipDecoder();
    virtual ~GZipDecoder() override;

    const GZipData& member_data() const { return m_member_data; }

private:
    Generator<StreamResult> decode();

    DeflateDecoder m_deflate_decoder;
    GZipData m_member_data;
};

class GZipEncoder final : public StreamEncoder {
public:
    GZipEncoder();
    virtual ~GZipEncoder() override;

    void set_name(String name) { m_gzip_data.name = move(name); }
    void set_comment(String comment) { m_gzip_data.comment = move(comment); }
    void set_time_last_modified(time_t time) { m_gzip_data.time_last_modified = time; }

private:
    Generator<StreamResult> encode();

    DeflateEncoder m_deflate_encoder;
    GZipData m_gzip_data;
};
}
