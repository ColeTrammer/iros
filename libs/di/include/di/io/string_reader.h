#pragma once

#include <di/container/algorithm/copy.h>
#include <di/container/string/encoding.h>
#include <di/container/string/mutable_string.h>
#include <di/container/string/string.h>
#include <di/container/string/utf8_encoding.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>
#include <di/meta/vocab.h>
#include <di/util/bit_cast.h>
#include <di/util/declval.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/span/as_bytes.h>

namespace di::io {
template<concepts::detail::ConstantString String>
class StringReader {
public:
    template<typename U>
    requires(concepts::ConstructibleFrom<String, U>)
    constexpr explicit StringReader(U&& buffer) : m_buffer(util::forward<U>(buffer)) {}

    constexpr usize read_some(vocab::Span<byte> data) {
        auto to_read = container::min(data.size(), m_buffer.size_bytes() - m_byte_offset);

        for (auto i : view::range(to_read)) {
            data[i] = read_byte();
        }

        return to_read;
    }

private:
    constexpr byte read_byte() {
        using CodeUnit = meta::EncodingCodeUnit<meta::Encoding<String>>;
        constexpr auto code_unit_size = sizeof(CodeUnit);

        auto word_index = m_byte_offset / code_unit_size;
        auto byte_index = m_byte_offset % code_unit_size;

        auto code_units = m_buffer.span();
        auto code_unit = code_units[word_index];
        auto as_bytes = util::bit_cast<vocab::Array<byte, code_unit_size>>(code_unit);

        m_byte_offset++;
        return as_bytes[byte_index];
    }

    String m_buffer;
    usize m_byte_offset { 0 };
};

template<typename String>
StringReader(String&&) -> StringReader<String>;
}
