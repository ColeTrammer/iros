#pragma once

#include <liim/container/strings/algorithm.h>

namespace LIIM::Container::Strings {
template<Encoding Enc>
class StringViewImpl;

template<typename Self, Encoding Enc>
class ReadonlyStringInterface {
public:
    using Iterator = EncodingIteratorType<Enc>;

    constexpr auto front() const { return Algorithm::front(Enc(), static_cast<Self const&>(*this).span()); }
    constexpr auto back() const { return Algorithm::back(Enc(), static_cast<Self const&>(*this).span()); }

    constexpr auto empty() const { return Algorithm::empty(Enc(), static_cast<Self const&>(*this).span()); }
    constexpr auto size_in_code_points() const { return Algorithm::size_in_code_points(Enc(), static_cast<Self const&>(*this).span()); }

    constexpr auto starts_with(Span<EncodingCodeUnitType<Enc> const> b) const {
        return Algorithm::starts_with(Enc(), static_cast<Self const&>(*this).span(), b);
    }
    constexpr auto ends_with(Span<EncodingCodeUnitType<Enc> const> b) const {
        return Algorithm::ends_with(Enc(), static_cast<Self const&>(*this).span(), b);
    }
    constexpr auto contains(Span<EncodingCodeUnitType<Enc> const> b) const {
        return Algorithm::contains(Enc(), static_cast<Self const&>(*this).span(), b);
    }

    constexpr auto begin() const {
        auto [begin, end] = Enc::code_point_iterators(static_cast<Self const&>(*this).span());
        return begin;
    }
    constexpr auto end() const {
        auto [begin, end] = Enc::code_point_iterators(static_cast<Self const&>(*this).span());
        return end;
    }

    constexpr StringViewImpl<Enc> substr(Iterator start, Option<Iterator> end = {}) const {
        auto start_offset = start.current_code_unit_offset();
        auto end_offset = end.value_or(this->end()).current_code_unit_offset();
        return StringViewImpl<Enc>(AssumeProperlyEncoded {},
                                   static_cast<Self const&>(*this).span().subspan(start_offset, end_offset - start_offset));
    }

    constexpr Option<Iterator> iterator_at_offset(size_t offset) const {
        return Enc::iterator_at_offset(static_cast<Self const&>(*this).span(), offset);
    }

    constexpr friend bool operator==(Self const& a, Self const& b) { return Algorithm::equal(Enc(), a.span(), b.span()); }
    constexpr friend std::strong_ordering operator<=>(Self const& a, Self const& b) {
        return Algorithm::compare(Enc(), a.span(), b.span());
    }
};
}
