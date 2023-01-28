#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/meta/enable_view.h>
#include <di/container/meta/prelude.h>
#include <di/container/string/constant_string.h>
#include <di/container/string/constant_string_interface.h>
#include <di/util/to_address.h>

namespace di::container::string {
template<concepts::Encoding Enc>
class StringViewImpl
    : public ConstantStringInterface<StringViewImpl<Enc>, Enc>
    , public meta::EnableView<StringViewImpl<Enc>>
    , public meta::EnableBorrowedContainer<StringViewImpl<Enc>> {
public:
    using Encoding = Enc;
    using CodeUnit = meta::EncodingCodeUnit<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using Iterator = meta::EncodingIterator<Enc>;

    StringViewImpl() = default;
    StringViewImpl(StringViewImpl const&) = default;

    constexpr StringViewImpl(Iterator begin, Iterator end, Enc encoding = {}) : m_encoding(encoding) {
        auto data = static_cast<CodeUnit const*>(begin);
        auto last = static_cast<CodeUnit const*>(end);
        m_data = data;
        m_size = (last - data);
    }

    template<concepts::detail::ConstantString Other>
    requires(!concepts::RemoveCVRefSameAs<StringViewImpl, Other> && concepts::SameAs<meta::Encoding<Other>, Enc> &&
             concepts::BorrowedContainer<Other>)
    constexpr StringViewImpl(Other&& other)
        : m_data(other.span().data()), m_size(other.span().size()), m_encoding(other.encoding()) {}

    constexpr StringViewImpl(CodeUnit const* data, size_t count, Enc encoding = {})
    requires(encoding::Universal<Enc>)
        : m_data(data), m_size(count), m_encoding(encoding) {}

    template<concepts::ContiguousIterator It, concepts::SizedSentinelFor<It> Sent>
    requires(!concepts::SameAs<It, Iterator> && concepts::SameAs<meta::IteratorValue<It>, CodeUnit> &&
             !concepts::ConvertibleTo<Sent, size_t> && encoding::Universal<Enc>)
    constexpr StringViewImpl(It it, Sent sent, Enc encoding = {})
        : m_data(util::to_address(it)), m_size(sent - it), m_encoding(encoding) {}

    template<concepts::ContiguousContainer Con>
    requires(!concepts::RemoveCVRefSameAs<StringViewImpl, Con> &&
             (!concepts::detail::ConstantString<Con> || !concepts::SameAs<meta::Encoding<Con>, Enc>) &&
             concepts::SizedContainer<Con> && concepts::ContainerOf<Con, CodeUnit> &&
             concepts::BorrowedContainer<Con> && encoding::Universal<Enc>)
    constexpr StringViewImpl(Con&& container, Enc encoding = {})
        : m_data(container::data(container)), m_size(container::size(container)), m_encoding(encoding) {}

    constexpr StringViewImpl(encoding::AssumeValid, CodeUnit const* data, size_t count, Enc encoding = {})
        : m_data(data), m_size(count), m_encoding(encoding) {}

    template<concepts::ContiguousIterator It, concepts::SizedSentinelFor<It> Sent>
    requires(concepts::SameAs<meta::IteratorValue<It>, CodeUnit> && !concepts::ConvertibleTo<Sent, size_t>)
    constexpr StringViewImpl(encoding::AssumeValid, It it, Sent sent, Enc encoding = {})
        : m_data(util::to_address(it)), m_size(sent - it), m_encoding(encoding) {}

    template<concepts::ContiguousContainer Con>
    requires(concepts::SizedContainer<Con> && concepts::ContainerOf<Con, CodeUnit> && concepts::BorrowedContainer<Con>)
    constexpr StringViewImpl(encoding::AssumeValid, Con&& container, Enc encoding = {})
        : m_data(container::data(container)), m_size(container::size(container)), m_encoding(encoding) {}

    StringViewImpl& operator=(StringViewImpl const&) = default;

    constexpr auto span() const { return Span { m_data, m_size }; }
    constexpr Enc encoding() const { return m_encoding; }

    constexpr void replace_begin(Iterator new_begin) { *this = { new_begin, this->end(), m_encoding }; }
    constexpr void replace_end(Iterator new_end) { *this = { this->begin(), new_end, m_encoding }; }

private:
    CodeUnit const* m_data { nullptr };
    size_t m_size { 0 };
    [[no_unique_address]] Enc m_encoding;
};
}
