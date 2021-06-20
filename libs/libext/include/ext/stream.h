#pragma once

#include <liim/byte_io.h>
#include <liim/generator.h>
#include <liim/string.h>

namespace Ext {
enum class StreamResult {
    Success,
    Error,
    NeedsMoreInput,
    NeedsMoreOutputSpace,
};

enum class StreamFlushMode {
    NoFlush,
    BlockFlush,
    StreamFlush,
};

class Stream {
public:
    Stream();
    virtual ~Stream();

    ByteWriter& writer() { return m_writer; }
    const ByteWriter& writer() const { return m_writer; }

    ByteReader& reader() { return m_reader; }
    const ByteReader& reader() const { return m_reader; }

    void set_output(Span<uint8_t> output) { m_writer.set_output(output); }
    void did_flush_output() { m_writer.set_offset(0); }

protected:
    Generator<StreamResult> write_bits(uint32_t bits, uint8_t bit_count);
    Generator<StreamResult> write_bytes(Span<const uint8_t> input);
    Generator<StreamResult> write_null_terminated_string(const String& string);

    Generator<StreamResult> read_bits(uint32_t& bits, uint8_t bit_count);
    Generator<StreamResult> read_bytes(Span<uint8_t> bytes);
    Generator<StreamResult> read_null_terminated_string(String& string);

private:
    ByteReader m_reader;
    ByteWriter m_writer;
};
}
