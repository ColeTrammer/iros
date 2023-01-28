#pragma once

#include <liim/container/strings/algorithm.h>

namespace LIIM::Container::Strings {
template<Encoding Enc>
class StringViewImpl;

template<typename Self, Encoding Enc>
class ReadonlyStringInterface {
public:
    using Iterator = EncodingIterator<Enc>;
    using CodeUnit = EncodingCodeUnit<Enc>;
    using CodePoint = EncodingCodePoint<Enc>;

    constexpr Span<CodeUnit const> span() const { return static_cast<Self const&>(*this).span(); }

    constexpr operator Span<CodeUnit const>() const { return span(); }

    constexpr auto data() const { return span().data(); }
    constexpr auto empty() const { return Algorithm::empty(Enc(), span()); }
    constexpr auto size_in_bytes() const { return span().size_in_bytes(); }
    constexpr auto size_in_code_units() const { return span().size(); }
    constexpr auto size_in_code_points() const { return Algorithm::size_in_code_points(Enc(), span()); }

    constexpr auto front() const { return Algorithm::front(Enc(), span()); }
    constexpr auto back() const { return Algorithm::back(Enc(), span()); }

    constexpr auto starts_with(CodePoint needle) const { return Algorithm::starts_with(Enc(), span(), needle); }
    constexpr auto starts_with(Span<CodeUnit const> needle) const { return Algorithm::starts_with(Enc(), span(), needle); }

    constexpr auto ends_with(CodePoint needle) const { return Algorithm::ends_with(Enc(), span(), needle); }
    constexpr auto ends_with(Span<CodeUnit const> needle) const { return Algorithm::ends_with(Enc(), span(), needle); }

    constexpr auto contains(CodePoint needle) const { return Algorithm::contains(Enc(), span(), needle); }
    constexpr auto contains(Span<CodeUnit const> needle) const { return Algorithm::contains(Enc(), span(), needle); }

    constexpr auto find(CodePoint needle) { return Algorithm::find(Enc(), span(), needle); }
    constexpr auto find(Span<CodeUnit const> needle) { return Algorithm::find(Enc(), span(), needle); }

    constexpr auto rfind(CodePoint needle) { return Algorithm::rfind(Enc(), span(), needle); }
    constexpr auto rfind(Span<CodeUnit const> needle) { return Algorithm::rfind(Enc(), span(), needle); }

    constexpr auto find_first_of(Span<CodeUnit const> needle) { return Algorithm::find_first_of(Enc(), span(), needle); }
    constexpr auto find_first_not_of(Span<CodeUnit const> needle) { return Algorithm::find_first_not_of(Enc(), span(), needle); }
    constexpr auto find_last_of(Span<CodeUnit const> needle) { return Algorithm::find_last_of(Enc(), span(), needle); }
    constexpr auto find_last_not_of(Span<CodeUnit const> needle) { return Algorithm::find_last_not_of(Enc(), span(), needle); }

    constexpr auto begin() const {
        auto [begin, end] = Enc::code_point_iterators(span());
        return begin;
    }
    constexpr auto end() const {
        auto [begin, end] = Enc::code_point_iterators(span());
        return end;
    }

    constexpr StringViewImpl<Enc> substr(Iterator start, Option<Iterator> end = {}) const {
        auto start_offset = Enc::iterator_code_unit_offset(span(), start);
        auto end_offset = Enc::iterator_code_unit_offset(span(), end.value_or(this->end()));
        return StringViewImpl<Enc>(AssumeProperlyEncoded {}, span().subspan(start_offset, end_offset - start_offset));
    }

    constexpr Option<Iterator> iterator_at_offset(size_t offset) const {
        return Enc::iterator_at_offset(static_cast<Self const&>(*this).span(), offset);
    }

    constexpr friend bool operator==(Self const& a, Self const& b) { return Algorithm::equal(Enc(), a.span(), b.span()); }
    constexpr friend bool operator==(Self const& a, Span<CodeUnit const> b) { return Algorithm::equal(Enc(), a.span(), b); }

    constexpr friend std::strong_ordering operator<=>(Self const& a, Self const& b) {
        return Algorithm::compare(Enc(), a.span(), b.span());
    }
    constexpr friend std::strong_ordering operator<=>(Self const& a, Span<CodeUnit const> b) {
        return Algorithm::compare(Enc(), a.span(), b);
    }
};
}
