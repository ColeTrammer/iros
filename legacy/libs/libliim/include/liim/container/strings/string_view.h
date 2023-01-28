#pragma once

#include <liim/container/strings/encoding.h>
#include <liim/container/strings/read_only_string_interface.h>
#include <liim/error.h>
#include <liim/span.h>

namespace LIIM::Container::Strings {
template<Encoding Enc>
class StringViewImpl : public ReadonlyStringInterface<StringViewImpl<Enc>, Enc> {
public:
    using CodeUnit = EncodingCodeUnit<Enc>;
    using CodePoint = EncodingCodePoint<Enc>;
    using Iterator = EncodingIterator<Enc>;

    constexpr static StringViewImpl create_unchecked_from_null_terminated_string(CodeUnit const* data) {
        size_t size_in_code_units = 0;
        while (data[size_in_code_units] != 0) {
            size_in_code_units++;
        }
        return StringViewImpl(AssumeProperlyEncoded {}, data, size_in_code_units);
    }

    constexpr StringViewImpl() = default;
    constexpr StringViewImpl(AssumeProperlyEncoded, CodeUnit const* data, size_t size_in_code_units)
        : m_data({ data, size_in_code_units }) {}
    constexpr StringViewImpl(AssumeProperlyEncoded, Span<CodeUnit const> data) : m_data(data) {}

    constexpr Span<CodeUnit const> span() const { return m_data; }

private:
    Span<CodeUnit const> m_data;
};
}
