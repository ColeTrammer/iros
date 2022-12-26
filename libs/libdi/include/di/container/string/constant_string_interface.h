#pragma once

#include <di/container/interface/reconstruct.h>
#include <di/container/string/constant_string.h>
#include <di/container/string/encoding.h>
#include <di/container/string/string_back.h>
#include <di/container/string/string_begin.h>
#include <di/container/string/string_compare.h>
#include <di/container/string/string_contains.h>
#include <di/container/string/string_data.h>
#include <di/container/string/string_empty.h>
#include <di/container/string/string_end.h>
#include <di/container/string/string_ends_with.h>
#include <di/container/string/string_equal.h>
#include <di/container/string/string_find.h>
#include <di/container/string/string_front.h>
#include <di/container/string/string_iterator_at_offset.h>
#include <di/container/string/string_rfind.h>
#include <di/container/string/string_size.h>
#include <di/container/string/string_size_code_units.h>
#include <di/container/string/string_starts_with.h>
#include <di/container/string/string_substr.h>
#include <di/container/string/string_unicode_code_points.h>
#include <di/parser/into_parser_context.h>
#include <di/parser/string_view_parser_context.h>

namespace di::container::string {
template<typename Self, concepts::Encoding Enc>
class ConstantStringInterface {
private:
    using CodeUnit = meta::EncodingCodeUnit<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using Iterator = meta::EncodingIterator<Enc>;

    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

public:
    constexpr size_t size() const
    requires(encoding::Contiguous<Enc>)
    {
        return string::size(self());
    }

    constexpr size_t size_bytes() const { return size_code_units() * sizeof(CodeUnit); }
    constexpr size_t size_code_units() const { return string::size_code_units(self()); }
    constexpr bool empty() const { return string::empty(self()); }

    constexpr auto data() const { return string::data(self()); }

    constexpr auto begin() const { return string::begin(self()); }
    constexpr auto end() const { return string::end(self()); }

    constexpr auto front() const { return string::front(self()); }
    constexpr auto back() const { return string::back(self()); }

    constexpr bool starts_with(CodePoint code_point) const { return string::starts_with(self(), code_point); }

    template<concepts::ContainerCompatible<CodePoint> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc>)
    constexpr bool starts_with(Con&& container) const {
        return string::starts_with(self(), util::forward<Con>(container));
    }

    constexpr bool ends_with(CodePoint code_point) const { return string::ends_with(self(), code_point); }

    template<concepts::ContainerCompatible<CodePoint> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc>)
    constexpr bool ends_with(Con&& container) const {
        return string::ends_with(self(), util::forward<Con>(container));
    }

    constexpr bool contains(CodePoint code_point) const { return string::contains(self(), code_point); }

    template<concepts::ContainerCompatible<CodePoint> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc> && concepts::ForwardContainer<Con>)
    constexpr bool contains(Con&& container) const {
        return string::contains(self(), util::forward<Con>(container));
    }

    constexpr auto substr(Iterator first, Optional<Iterator> last = {}) const { return string::substr(self(), first, last); }

    constexpr auto find(CodePoint code_point) const { return string::find(self(), code_point); }

    template<concepts::ContainerCompatible<CodePoint> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc> && concepts::ForwardContainer<Con>)
    constexpr auto find(Con&& container) const {
        return string::find(self(), util::forward<Con>(container));
    }

    constexpr auto rfind(CodePoint code_point) const { return string::rfind(self(), code_point); }

    template<concepts::ContainerCompatible<CodePoint> Con>
    requires(concepts::SameAs<meta::Encoding<Con>, Enc> && concepts::ForwardContainer<Con>)
    constexpr auto rfind(Con&& container) const {
        return string::rfind(self(), util::forward<Con>(container));
    }

    constexpr auto view() const { return StringViewImpl<Enc>(self()); }

    constexpr auto iterator_at_offset(size_t index) const { return string::iterator_at_offset(self(), index); }

    constexpr auto unicode_code_points() const { return string::unicode_code_points(self()); }

private:
    template<concepts::detail::ConstantString Other>
    requires(concepts::SameAs<Enc, meta::Encoding<Other>>)
    constexpr friend bool operator==(Self const& a, Other const& b) {
        return string::equal(a, b);
    }

    template<concepts::detail::ConstantString Other>
    requires(concepts::SameAs<Enc, meta::Encoding<Other>>)
    constexpr friend bool operator<=>(Self const& a, Other const& b) {
        return string::compare(a, b);
    }

    constexpr friend auto tag_invoke(types::Tag<parser::into_parser_context>, Self const& self) {
        return parser::StringViewParserContext<Enc>(self.view());
    }

    constexpr friend StringViewImpl<Enc> tag_invoke(types::Tag<container::reconstruct>, InPlaceType<Self>, Iterator first, Iterator last) {
        return StringViewImpl<Enc>(util::move(first), util::move(last));
    }
};
}
