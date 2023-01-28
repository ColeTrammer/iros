#pragma once

#include <di/assert/prelude.h>
#include <di/concepts/array.h>
#include <di/concepts/const.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/convertible_to_non_slicing.h>
#include <di/concepts/equality_comparable.h>
#include <di/concepts/language_array.h>
#include <di/concepts/span.h>
#include <di/concepts/three_way_comparable.h>
#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/container/concepts/borrowed_container.h>
#include <di/container/concepts/contiguous_container.h>
#include <di/container/concepts/contiguous_iterator.h>
#include <di/container/concepts/sized_container.h>
#include <di/container/concepts/sized_sentinel_for.h>
#include <di/container/interface/data.h>
#include <di/container/interface/reconstruct.h>
#include <di/container/interface/size.h>
#include <di/container/meta/container_reference.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/meta/enable_view.h>
#include <di/container/meta/iterator_reference.h>
#include <di/meta/add_member_get.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/remove_reference.h>
#include <di/types/size_t.h>
#include <di/util/get_in_place.h>
#include <di/util/to_address.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/span/span_forward_declaration.h>
#include <di/vocab/tuple/enable_generate_structed_bindings.h>
#include <di/vocab/tuple/tuple_element.h>
#include <di/vocab/tuple/tuple_size.h>

namespace di::vocab {
template<typename T, types::size_t extent>
requires(extent != dynamic_extent)
class Span<T, extent>
    : public meta::EnableView<Span<T, extent>>
    , public meta::EnableBorrowedContainer<Span<T, extent>>
    , public meta::AddMemberGet<Span<T, extent>> {
public:
    using Element = T;

    constexpr explicit Span()
    requires(extent == 0)
    = default;

    template<concepts::ContiguousIterator Iter>
    requires(concepts::ConvertibleToNonSlicing<meta::RemoveReference<meta::IteratorReference<Iter>>, T>)
    constexpr explicit Span(Iter first, types::size_t count) : m_data(util::to_address(first)) {
        DI_ASSERT(count == extent);
    }

    template<concepts::ContiguousIterator Iter, concepts::SizedSentinelFor<Iter> Sent>
    requires(concepts::ConvertibleToNonSlicing<meta::RemoveReference<meta::IteratorReference<Iter>>, T> &&
             !concepts::ConvertibleTo<Sent, types::size_t>)
    constexpr explicit Span(Iter it, Sent sent) : m_data(util::to_address(it)) {
        DI_ASSERT(sent - it == extent);
    }

    template<types::size_t size>
    requires(size == extent)
    constexpr Span(T (&array)[size]) : m_data(array) {}

    template<concepts::ConvertibleToNonSlicing<T> U, types::size_t size>
    requires(size == extent)
    constexpr Span(vocab::Array<U, size>& array) : m_data(array.data()) {}

    template<typename U, types::size_t size>
    requires(size == extent && concepts::ConvertibleToNonSlicing<U const, T>)
    constexpr Span(vocab::Array<U, size> const& array) : m_data(array.data()) {}

    template<concepts::ContiguousContainer Con>
    requires(concepts::SizedContainer<Con> && (concepts::BorrowedContainer<Con> || concepts::Const<T>) &&
             !concepts::Span<Con> && !concepts::Array<Con> && !concepts::LanguageArray<meta::RemoveCVRef<Con>> &&
             concepts::ConvertibleToNonSlicing<meta::RemoveReference<meta::ContainerReference<Con>>, T>)
    constexpr explicit Span(Con&& container) : m_data(container::data(container)) {
        DI_ASSERT(container::size(container) == extent);
    }

    template<concepts::ConvertibleToNonSlicing<T> U, types::size_t other_extent>
    requires((other_extent == dynamic_extent || extent == other_extent))
    constexpr explicit(other_extent == dynamic_extent) Span(Span<U, other_extent> const& other) : m_data(other.data()) {
        DI_ASSERT(other.size() == extent);
    }

    constexpr Span(Span const&) = default;

    constexpr Span& operator=(Span const&) = default;

    constexpr T* begin() const { return data(); }
    constexpr T* end() const { return data() + extent; }

    constexpr T& front() const
    requires(extent > 0)
    {
        return *data();
    }

    constexpr T& back() const
    requires(extent > 0)
    {
        return *(end() - 1);
    }

    constexpr vocab::Optional<T&> at(types::size_t index) const {
        if (index >= extent) {
            return nullopt;
        }
        return (*this)[index];
    }

    constexpr T& operator[](types::size_t index) const
    requires(extent > 0)
    {
        // DI_ASSSERT( index < extent )
        return data()[index];
    }

    constexpr T* data() const { return m_data; }

    constexpr types::size_t size() const { return extent; }
    constexpr types::size_t size_bytes() const { return sizeof(T) * extent; }

    [[nodiscard]] constexpr bool empty() const { return extent == 0; }

    constexpr Optional<Span<T>> first(types::size_t count) const {
        if (count > extent) {
            return nullopt;
        }
        return Span<T> { data(), count };
    }

    constexpr Optional<Span<T>> last(types::size_t count) const {
        if (count > extent) {
            return nullopt;
        }
        return Span<T> { end() - count, count };
    }

    constexpr Optional<Span<T>> subspan(types::size_t offset) const {
        if (offset > extent) {
            return nullopt;
        }
        return Span<T> { data() + offset, extent - offset };
    }

    constexpr Optional<Span<T>> subspan(types::size_t offset, types::size_t count) const {
        if (offset + count > extent) {
            return nullopt;
        }
        return Span<T> { data() + offset, count };
    }

    template<types::size_t count>
    requires(count <= extent)
    constexpr auto first() const {
        return Span<T, count> { data(), count };
    }

    template<types::size_t count>
    requires(count <= extent)
    constexpr auto last() const {
        return Span<T, count> { end() - count, count };
    }

    template<types::size_t offset, types::size_t count = dynamic_extent>
    requires(offset <= extent && (count == dynamic_extent || offset + count <= extent))
    constexpr auto subspan() const {
        if constexpr (count == dynamic_extent) {
            return Span<T, extent - offset> { data() + offset, end() };
        } else {
            return Span<T, count> { data() + offset, count };
        }
    }

private:
    constexpr friend bool operator==(Span a, Span b)
    requires(concepts::EqualityComparable<T>)
    {
        return container::equal(a, b);
    }

    constexpr friend auto operator<=>(Span a, Span b)
    requires(concepts::ThreeWayComparable<T>)
    {
        return container::compare(a, b);
    }

    template<concepts::ContiguousIterator It, concepts::SizedSentinelFor<It> Sent>
    requires(concepts::ConvertibleToNonSlicing<It, T*>)
    constexpr friend Span<T> tag_invoke(types::Tag<container::reconstruct>, InPlaceType<Span>, It first, Sent last) {
        return Span<T>(util::move(first), util::move(last));
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr friend InPlaceType<T> tag_invoke(types::Tag<tuple_element>, types::InPlaceType<Span>,
                                               types::InPlaceIndex<index>);

    template<types::size_t index>
    requires(index < extent)
    constexpr friend InPlaceType<T> tag_invoke(types::Tag<tuple_element>, types::InPlaceType<Span const>,
                                               types::InPlaceIndex<index>);

    constexpr friend types::size_t tag_invoke(types::Tag<tuple_size>, types::InPlaceType<Span>) { return extent; }

    template<types::size_t index>
    requires(index < extent)
    constexpr friend T& tag_invoke(types::Tag<util::get_in_place>, types::InPlaceIndex<index>, Span self) {
        return self[index];
    }

    T* m_data { nullptr };
};

template<typename T, types::size_t size>
Span(T (&)[size]) -> Span<T, size>;

template<typename T, types::size_t size>
Span(vocab::Array<T, size>&) -> Span<T, size>;

template<typename T, types::size_t size>
Span(vocab::Array<T, size> const&) -> Span<T const, size>;
}
