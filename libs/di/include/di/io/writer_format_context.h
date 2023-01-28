#pragma once

#include <di/format/prelude.h>
#include <di/io/concepts/writer.h>
#include <di/util/bit_cast.h>
#include <di/vocab/array/prelude.h>

namespace di::io {
template<concepts::Writer Writer, concepts::Encoding Enc>
class WriterFormatContext {
public:
    using Encoding = Enc;

    constexpr explicit WriterFormatContext(Writer& writer, Enc enc) : m_writer(writer), m_encoding(enc) {}

    constexpr ~WriterFormatContext() { (void) m_writer.flush(); }

    constexpr void output(meta::EncodingCodePoint<Enc> code_point) {
        auto code_units = container::string::encoding::convert_to_code_units(m_encoding, code_point);

        for (auto code_unit : code_units) {
            auto bytes = util::bit_cast<Array<Byte, sizeof(code_unit)>>(code_unit);
            (void) m_writer.write_some(bytes.span());
        }
    }

    constexpr auto encoding() const { return m_encoding; }

private:
    Writer& m_writer;
    [[no_unique_address]] Enc m_encoding;
};
}