#pragma once

#include <liim/container/strings/encoding.h>
#include <liim/container/strings/read_only_string_interface.h>
#include <liim/error.h>
#include <liim/span.h>

namespace LIIM::Container::Strings {
template<Encoding Enc>
class StringViewImpl : public ReadonlyStringInterface<StringViewImpl<Enc>, Enc> {
public:
    using CodeUnitType = EncodingCodeUnitType<Enc>;
    using CodePointType = EncodingCodePointType<Enc>;
    using Iterator = EncodingIteratorType<Enc>;

    constexpr static StringViewImpl create_unchecked_from_null_terminated_string(CodeUnitType const* data) {
        size_t size_in_code_units = 0;
        while (data[size_in_code_units] != 0) {
            size_in_code_units++;
        }
        return StringViewImpl(AssumeProperlyEncoded {}, data, size_in_code_units);
    }

    constexpr StringViewImpl() = default;
    constexpr StringViewImpl(AssumeProperlyEncoded, CodeUnitType const* data, size_t size_in_code_units)
        : m_data({ data, size_in_code_units }) {}
    constexpr StringViewImpl(AssumeProperlyEncoded, Span<CodeUnitType const> data) : m_data(data) {}

    constexpr StringViewImpl(StringViewImpl const&) = default;
    constexpr StringViewImpl(StringViewImpl&&) = default;

    constexpr StringViewImpl& operator=(StringViewImpl const&) = default;
    constexpr StringViewImpl& operator=(StringViewImpl&&) = default;

    constexpr operator Span<CodeUnitType const>() const { return span(); }

    constexpr CodeUnitType const* data() const { return m_data.data(); }
    constexpr size_t size_in_bytes() const { return size_in_code_units() * sizeof(CodeUnitType); }
    constexpr size_t size_in_code_units() const { return m_data.size(); }

    constexpr Span<CodeUnitType const> span() const { return { data(), size_in_bytes() }; }

private:
    Span<CodeUnitType const> m_data;
};
}
