#pragma once

#include <di/container/string/mutable_string_interface.h>
#include <di/container/vector/vector.h>

namespace di::container::string {
template<concepts::Encoding Enc>
class StringImpl : public MutableStringInterface<StringImpl<Enc>, Enc> {
public:
    using Encoding = Enc;
    using CodeUnit = meta::EncodingCodeUnit<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using Iterator = meta::EncodingIterator<Enc>;
    using Value = CodeUnit;
    using ConstValue = CodeUnit const;

    constexpr auto span() { return m_vector.span(); }
    constexpr auto span() const { return m_vector.span(); }

    constexpr Enc encoding() const { return m_encoding; }

    constexpr auto capacity() const { return m_vector.capacity(); }
    constexpr auto max_size() const { return m_vector.max_size(); }
    constexpr auto reserve_from_nothing(size_t n) { return m_vector.reserve_from_nothing(n); }
    constexpr auto assume_size(size_t n) { return m_vector.assume_size(n); }

private:
    Vector<CodeUnit> m_vector;
    [[no_unique_address]] Enc m_encoding;
};
}
