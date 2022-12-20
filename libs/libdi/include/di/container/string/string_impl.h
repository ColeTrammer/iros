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

    StringImpl() = default;

    constexpr auto span() { return m_vector.span(); }
    constexpr auto span() const { return m_vector.span(); }

    constexpr Enc encoding() const { return m_encoding; }

    constexpr auto capacity() const { return m_vector.capacity(); }
    constexpr auto max_size() const { return m_vector.max_size(); }
    constexpr auto reserve_from_nothing(size_t n) { return m_vector.reserve_from_nothing(n); }
    constexpr auto assume_size(size_t n) { return m_vector.assume_size(n); }

private:
    constexpr explicit StringImpl(Vector<CodeUnit>&& storage) : m_vector(util::move(storage)) {}

    constexpr friend auto tag_invoke(types::Tag<util::create_in_place>, InPlaceType<StringImpl>, Vector<CodeUnit>&& storage) {
        if constexpr (encoding::universal(in_place_type<Enc>)) {
            return StringImpl { util::move(storage) };
        } else {
            DI_ASSERT(encoding::validate(Enc(), util::as_const(storage).span()));
            return StringImpl { util::move(storage) };
        }
    }

    Vector<CodeUnit> m_vector;
    [[no_unique_address]] Enc m_encoding {};
};
}
