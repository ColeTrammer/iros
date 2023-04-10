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
    using SupportsStyle = void;

    constexpr explicit WriterFormatContext(Writer& writer, Enc enc) : m_writer(writer), m_encoding(enc) {
        if constexpr (requires { writer.interactive_device(); }) {
            m_print_colors = writer.interactive_device();
        }
    }

    constexpr ~WriterFormatContext() { (void) flush(m_writer); }

    constexpr void output(meta::EncodingCodePoint<Enc> code_point) {
        auto code_units = container::string::encoding::convert_to_code_units(m_encoding, code_point);

        for (auto code_unit : code_units) {
            auto bytes = util::bit_cast<Array<Byte, sizeof(code_unit)>>(code_unit);
            (void) write_some(m_writer, bytes.span());
        }
    }

    constexpr Result<void> with_style(format::Style style, concepts::InvocableTo<Result<void>> auto inner) {
        if (!m_print_colors) {
            return inner();
        }

        auto [before, after] = style.render_to_ansi_escapes<Enc>();
        for (auto code_point : before) {
            output(code_point);
        }
        DI_TRY(inner());
        for (auto code_point : after) {
            output(code_point);
        }
        return {};
    }

    constexpr auto encoding() const { return m_encoding; }

private:
    Writer& m_writer;
    [[no_unique_address]] Enc m_encoding;
    bool m_print_colors { false };
};
}
