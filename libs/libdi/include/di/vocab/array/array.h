#pragma once

#include <di/assert/prelude.h>
#include <di/concepts/copyable.h>
#include <di/concepts/decay_same_as.h>
#include <di/concepts/equality_comparable.h>
#include <di/concepts/three_way_comparable.h>
#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/types/size_t.h>
#include <di/util/forward_like.h>
#include <di/util/get_in_place.h>
#include <di/util/move.h>
#include <di/util/swap.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/span/prelude.h>
#include <di/vocab/tuple/enable_generate_structed_bindings.h>
#include <di/vocab/tuple/tuple_element.h>
#include <di/vocab/tuple/tuple_size.h>

namespace di::vocab {
template<typename T, types::size_t extent>
struct Array {
public:
    T m_public_data[extent];

    constexpr Optional<T&> at(types::size_t index) {
        if (index >= extent) {
            return nullopt;
        }
        return (*this)[index];
    }
    constexpr Optional<T const&> at(types::size_t index) const {
        if (index >= extent) {
            return nullopt;
        }
        return (*this)[index];
    }

    constexpr T& operator[](types::size_t index) {
        DI_ASSERT_LT(index, extent);
        return begin()[index];
    }
    constexpr T const& operator[](types::size_t index) const {
        DI_ASSERT_LT(index, extent);
        return begin()[index];
    }

    constexpr T& front()
    requires(extent > 0)
    {
        return *begin();
    }
    constexpr T const& front() const
    requires(extent > 0)
    {
        return *begin();
    }

    constexpr T& back()
    requires(extent > 0)
    {
        return *(end() - 1);
    }
    constexpr T const& back() const
    requires(extent > 0)
    {
        return *(end() - 1);
    }

    constexpr T* data() { return m_public_data; }
    constexpr T const* data() const { return m_public_data; }

    constexpr T* begin() { return data(); }
    constexpr T const* begin() const { return data(); }

    constexpr T* end() { return data() + extent; }
    constexpr T const* end() const { return data() + extent; }

    constexpr bool empty() const { return extent == 0; }
    constexpr auto size() const { return extent; }
    constexpr auto max_size() const { return extent; }

    constexpr void fill(T const& value)
    requires(concepts::Copyable<T>)
    {
        for (auto& lvalue : *this) {
            lvalue = value;
        }
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
    constexpr T& get() & {
        return (*this)[index];
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr T const& get() const& {
        return (*this)[index];
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr T&& get() && {
        return util::move((*this)[index]);
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr T const&& get() const&& {
        return util::move((*this)[index]);
    }

private:
    constexpr friend bool operator==(Array const& a, Array const& b)
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
        for (types::size_t i = 0; i < extent; i++) {
            util::swap(a[i], b[i]);
        }
    }

    constexpr friend bool tag_invoke(types::Tag<vocab::enable_generate_structed_bindings>, types::InPlaceType<Array>) { return true; }
    constexpr friend types::size_t tag_invoke(types::Tag<vocab::tuple_size>, types::InPlaceType<Array>) { return extent; }

    template<types::size_t index>
    requires(index < extent)
    constexpr friend T tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<Array>, types::InPlaceIndex<index>);

    template<types::size_t index>
    requires(index < extent)
    constexpr friend T const tag_invoke(types::Tag<vocab::tuple_element>, types::InPlaceType<Array const>, types::InPlaceIndex<index>);

    template<concepts::DecaySameAs<Array> Self, types::size_t index>
    requires(index < extent)
    constexpr friend decltype(auto) tag_invoke(types::Tag<util::get_in_place>, types::InPlaceIndex<index>, Self&& self) {
        return util::forward_like<Self>(self[index]);
    }
};

template<typename T, typename... U>
Array(T, U...) -> Array<T, 1 + sizeof...(U)>;
}
