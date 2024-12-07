#pragma once

#include "di/assert/assert_bool.h"
#include "di/container/algorithm/compare.h"
#include "di/container/algorithm/equal.h"
#include "di/container/algorithm/fill.h"
#include "di/container/algorithm/swap_ranges.h"
#include "di/container/interface/reconstruct.h"
#include "di/meta/compare.h"
#include "di/meta/constexpr.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/types/size_t.h"
#include "di/util/forward_like.h"
#include "di/util/get_in_place.h"
#include "di/util/move.h"
#include "di/util/swap.h"
#include "di/util/unreachable.h"
#include "di/vocab/optional/prelude.h"
#include "di/vocab/span/prelude.h"
#include "di/vocab/tuple/enable_generate_structed_bindings.h"
#include "di/vocab/tuple/tuple_element.h"
#include "di/vocab/tuple/tuple_size.h"

namespace di::vocab {
template<typename T, types::size_t extent>
struct Array {
public:
    T m_public_data[extent];

    constexpr auto at(types::size_t index) -> Optional<T&> {
        if (index >= extent) {
            return nullopt;
        }
        return (*this)[index];
    }
    constexpr auto at(types::size_t index) const -> Optional<T const&> {
        if (index >= extent) {
            return nullopt;
        }
        return (*this)[index];
    }

    constexpr auto operator[](types::size_t index) -> T& {
        DI_ASSERT(index < extent);
        return begin()[index];
    }
    constexpr auto operator[](types::size_t index) const -> T const& {
        DI_ASSERT(index < extent);
        return begin()[index];
    }

    constexpr auto front() -> T&
    requires(extent > 0)
    {
        return *begin();
    }
    constexpr auto front() const -> T const&
    requires(extent > 0)
    {
        return *begin();
    }

    constexpr auto back() -> T&
    requires(extent > 0)
    {
        return *(end() - 1);
    }
    constexpr auto back() const -> T const&
    requires(extent > 0)
    {
        return *(end() - 1);
    }

    constexpr auto data() -> T* { return m_public_data; }
    constexpr auto data() const -> T const* { return m_public_data; }

    constexpr auto begin() -> T* { return data(); }
    constexpr auto begin() const -> T const* { return data(); }

    constexpr auto end() -> T* { return data() + extent; }
    constexpr auto end() const -> T const* { return data() + extent; }

    constexpr auto empty() const -> bool { return extent == 0; }
    constexpr auto size() const { return extent; }
    constexpr auto max_size() const { return extent; }

    constexpr void fill(T const& value)
    requires(concepts::Copyable<T>)
    {
        container::fill(*this, value);
    }

    constexpr auto span() { return Span { *this }; }
    constexpr auto span() const { return Span { *this }; }

    constexpr auto first(types::size_t count) { return span().first(count); }
    constexpr auto first(types::size_t count) const { return span().first(count); }

    constexpr auto last(types::size_t count) { return span().last(count); }
    constexpr auto last(types::size_t count) const { return span().last(count); }

    constexpr auto subspan(types::size_t offset) { return span().subspan(offset); }
    constexpr auto subspan(types::size_t offset) const { return span().subspan(offset); }

    constexpr auto subspan(types::size_t offset, types::size_t count) { return span().subspan(offset, count); }
    constexpr auto subspan(types::size_t offset, types::size_t count) const { return span().subspan(offset, count); }

    template<types::size_t count>
    requires(count <= extent)
    constexpr auto first() {
        return this->span().template first<count>();
    }

    template<types::size_t count>
    requires(count <= extent)
    constexpr auto first() const {
        return this->span().template first<count>();
    }

    template<types::size_t count>
    requires(count <= extent)
    constexpr auto last() {
        return this->span().template last<count>();
    }

    template<types::size_t count>
    requires(count <= extent)
    constexpr auto last() const {
        return this->span().template last<count>();
    }

    template<types::size_t offset, types::size_t count = dynamic_extent>
    requires(offset <= extent && (count == dynamic_extent || offset + count <= extent))
    constexpr auto subspan() {
        return this->span().template subspan<offset, count>();
    }

    template<types::size_t offset, types::size_t count = dynamic_extent>
    requires(offset <= extent && (count == dynamic_extent || offset + count <= extent))
    constexpr auto subspan() const {
        return this->span().template subspan<offset, count>();
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr auto get() & -> T& {
        return (*this)[index];
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr auto get() const& -> T const& {
        return (*this)[index];
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr auto get() && -> T&& {
        return util::move((*this)[index]);
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr auto get() const&& -> T const&& {
        return util::move((*this)[index]);
    }

private:
    constexpr friend auto operator==(Array const& a, Array const& b) -> bool
    requires(concepts::EqualityComparable<T>)
    {
        return container::equal(a, b);
    }

    constexpr friend auto operator<=>(Array const& a, Array const& b)
    requires(concepts::ThreeWayComparable<T>)
    {
        return container::compare(a, b);
    }

    constexpr friend void tag_invoke(types::Tag<util::swap>, Array& a, Array& b)
    requires(concepts::Swappable<T>)
    {
        container::swap_ranges(a, b);
    }

    constexpr friend auto tag_invoke(types::Tag<vocab::enable_generate_structed_bindings>, types::InPlaceType<Array>)
        -> bool {
        return true;
    }
    constexpr friend auto tag_invoke(types::Tag<vocab::tuple_size>, types::InPlaceType<Array>) -> types::size_t {
        return extent;
    }

    template<concepts::ContiguousIterator It, concepts::SizedSentinelFor<It> Sent>
    requires(concepts::ConvertibleToNonSlicing<It, T*>)
    constexpr friend auto tag_invoke(types::Tag<container::reconstruct>, InPlaceType<Array>, It first, Sent last)
        -> Span<T> {
        return Span<T>(util::move(first), util::move(last));
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr friend auto tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<Array>, Constexpr<index>)
        -> InPlaceType<T> {
        return {};
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr friend auto tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<Array const>,
                                     Constexpr<index>) -> InPlaceType<T const> {
        return {};
    }

    template<concepts::DecaySameAs<Array> Self, types::size_t index>
    requires(index < extent)
    constexpr friend auto tag_invoke(types::Tag<util::get_in_place>, Constexpr<index>, Self&& self) -> decltype(auto) {
        return util::forward_like<Self>(self.data()[index]);
    }
};

template<typename T>
struct Array<T, 0> {
    constexpr auto at(types::size_t) -> Optional<T&> { return nullopt; }
    constexpr auto at(types::size_t) const -> Optional<T const&> { return nullopt; }

    constexpr auto data() -> T* { return nullptr; }
    constexpr auto data() const -> T const* { return nullptr; }

    constexpr auto begin() -> T* { return data(); }
    constexpr auto begin() const -> T const* { return data(); }

    constexpr auto end() -> T* { return data(); }
    constexpr auto end() const -> T const* { return data(); }

    constexpr auto operator[](types::size_t) -> T& {
        DI_ASSERT(false);
        util::unreachable();
    }
    constexpr auto operator[](types::size_t) const -> T const& {
        DI_ASSERT(false);
        util::unreachable();
    }

    constexpr auto empty() const -> bool { return false; }
    constexpr auto size() const { return 0ZU; }
    constexpr auto max_size() const { return 0; }

    constexpr void fill(T const&)
    requires(concepts::Copyable<T>)
    {}

    constexpr auto span() { return Span { *this }; }
    constexpr auto span() const { return Span { *this }; }

private:
    constexpr friend auto operator==(Array const&, Array const&) -> bool
    requires(concepts::EqualityComparable<T>)
    {
        return true;
    }

    constexpr friend auto operator<=>(Array const&, Array const&)
    requires(concepts::ThreeWayComparable<T>)
    {
        return strong_ordering::equal;
    }

    constexpr friend void tag_invoke(types::Tag<util::swap>, Array&, Array&)
    requires(concepts::Swappable<T>)
    {}

    constexpr friend auto tag_invoke(types::Tag<vocab::enable_generate_structed_bindings>, types::InPlaceType<Array>)
        -> bool {
        return true;
    }
    constexpr friend auto tag_invoke(types::Tag<vocab::tuple_size>, types::InPlaceType<Array>) -> types::size_t {
        return 0;
    }

    template<concepts::ContiguousIterator It, concepts::SizedSentinelFor<It> Sent>
    requires(concepts::ConvertibleToNonSlicing<It, T*>)
    constexpr friend auto tag_invoke(types::Tag<container::reconstruct>, InPlaceType<Array>, It first, Sent last)
        -> Span<T> {
        return Span<T>(util::move(first), util::move(last));
    }
};

template<typename T, typename... U>
Array(T, U...) -> Array<T, 1 + sizeof...(U)>;
}

namespace di {
using vocab::Array;
}
