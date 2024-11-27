#pragma once

#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/container/interface/reconstruct.h>
#include <di/container/vector/constant_vector.h>
#include <di/container/vector/vector_at.h>
#include <di/container/vector/vector_back.h>
#include <di/container/vector/vector_begin.h>
#include <di/container/vector/vector_data.h>
#include <di/container/vector/vector_empty.h>
#include <di/container/vector/vector_end.h>
#include <di/container/vector/vector_first.h>
#include <di/container/vector/vector_front.h>
#include <di/container/vector/vector_iterator.h>
#include <di/container/vector/vector_last.h>
#include <di/container/vector/vector_lookup.h>
#include <di/container/vector/vector_size.h>
#include <di/container/vector/vector_size_bytes.h>
#include <di/container/vector/vector_subspan.h>
#include <di/meta/compare.h>
#include <di/types/prelude.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/span/span_fixed_size.h>

namespace di::container {
template<typename Self, typename Value>
class ConstantVectorInterface {
private:
    constexpr auto self() -> Self& { return static_cast<Self&>(*this); }
    constexpr auto self() const -> Self const& { return static_cast<Self const&>(*this); }

public:
    constexpr auto size() const -> size_t { return vector::size(self()); }
    constexpr auto size_bytes() const -> size_t { return vector::size_bytes(self()); }
    [[nodiscard]] constexpr auto empty() const -> bool { return vector::empty(self()); }

    constexpr auto begin() { return vector::begin(self()); }
    constexpr auto begin() const { return vector::begin(self()); }

    constexpr auto end() { return vector::end(self()); }
    constexpr auto end() const { return vector::end(self()); }

    constexpr auto front() { return vector::front(self()); }
    constexpr auto front() const { return vector::front(self()); }

    constexpr auto back() { return vector::back(self()); }
    constexpr auto back() const { return vector::back(self()); }

    constexpr auto at(size_t index) { return vector::at(self(), index); }
    constexpr auto at(size_t index) const { return vector::at(self(), index); }

    constexpr auto operator[](size_t index) -> decltype(auto) { return vector::lookup(self(), index); }
    constexpr auto operator[](size_t index) const -> decltype(auto) { return vector::lookup(self(), index); }

    constexpr auto iterator(size_t index) { return vector::iterator(self(), index); }
    constexpr auto iterator(size_t index) const { return vector::iterator(self(), index); }
    constexpr auto citerator(size_t index) const { return vector::iterator(self(), index); }

    constexpr auto data() { return vector::data(self()); }
    constexpr auto data() const { return vector::data(self()); }

    constexpr auto first(size_t count) { return vector::first(self(), count); }
    constexpr auto first(size_t count) const { return vector::first(self(), count); }

    constexpr auto last(size_t count) { return vector::last(self(), count); }
    constexpr auto last(size_t count) const { return vector::last(self(), count); }

    constexpr auto subspan(size_t offset) { return vector::subspan(self(), offset); }
    constexpr auto subspan(size_t offset) const { return vector::subspan(self(), offset); }
    constexpr auto subspan(size_t offset, size_t count) { return vector::subspan(self(), offset, count); }
    constexpr auto subspan(size_t offset, size_t count) const { return vector::subspan(self(), offset, count); }

    template<size_t count>
    constexpr auto first() {
        return vector::first<count>(self());
    }

    template<size_t count>
    constexpr auto first() const {
        return vector::first<count>(self());
    }

    template<size_t count>
    constexpr auto last() {
        return vector::last<count>(self());
    }

    template<size_t count>
    constexpr auto last() const {
        return vector::last<count>(self());
    }

    template<size_t offset, size_t count = vocab::dynamic_extent>
    constexpr auto subspan() {
        return vector::subspan<offset, count>(self());
    }

    template<size_t offset, size_t count = vocab::dynamic_extent>
    constexpr auto subspan() const {
        return vector::subspan<offset, count>(self());
    }

private:
    constexpr friend auto operator==(Self const& a, Self const& b) -> bool
    requires(concepts::EqualityComparable<Value>)
    {
        return container::equal(a, b);
    }

    constexpr friend auto operator<=>(Self const& a, Self const& b)
    requires(concepts::ThreeWayComparable<Value>)
    {
        return container::compare(a, b);
    }

    template<concepts::ContiguousIterator It, concepts::SizedSentinelFor<It> Sent>
    requires(concepts::ConvertibleToNonSlicing<It, Value*>)
    constexpr friend auto tag_invoke(types::Tag<container::reconstruct>, InPlaceType<Self>, It first, Sent last)
        -> vocab::Span<Value> {
        return vocab::Span<Value>(util::move(first), util::move(last));
    }
};
}
