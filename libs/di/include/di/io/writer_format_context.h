#pragma once

#include <di/format/prelude.h>
#include <di/io/interface/writer.h>
#include <di/util/bit_cast.h>
#include <di/vocab/array/prelude.h>

namespace di::io {
template<Impl<Writer> Writer, concepts::Encoding Enc>
class WriterFormatContext {
public:
    using Encoding = Enc;

    constexpr explicit WriterFormatContext(Writer& writer, Enc enc) : m_writer(writer), m_encoding(enc) {}

    constexpr ~WriterFormatContext() { (void) flush(m_writer); }

    constexpr void output(meta::EncodingCodePoint<Enc> code_point) {
        auto code_units = container::string::encoding::convert_to_code_units(m_encoding, code_point);

        for (auto code_unit : code_units) {
            auto bytes = util::bit_cast<Array<Byte, sizeof(code_unit)>>(code_unit);
            (void) write_some(m_writer, bytes.span());
        }
    }

    constexpr auto encoding() const { return m_encoding; }

private:
    Writer& m_writer;
    [[no_unique_address]] Enc m_encoding;
};
}