#pragma once

#include <di/concepts/array.h>
#include <di/concepts/const.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/convertible_to_non_slicing.h>
#include <di/concepts/language_array.h>
#include <di/concepts/span.h>
#include <di/container/concepts/borrowed_container.h>
#include <di/container/concepts/contiguous_container.h>
#include <di/container/concepts/contiguous_iterator.h>
#include <di/container/concepts/sized_container.h>
#include <di/container/concepts/sized_sentinel_for.h>
#include <di/container/interface/data.h>
#include <di/container/interface/size.h>
#include <di/container/meta/container_value.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/meta/enable_view.h>
#include <di/container/meta/iterator_value.h>
#include <di/container/vector/span_forward_declaration.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/remove_reference.h>
#include <di/types/size_t.h>
#include <di/util/to_address.h>
#include <di/vocab/optional.h>

namespace di::container {
template<typename T, types::size_t extent>
requires(extent != dynamic_extent)
class Span<T, extent>
    : public meta::EnableView<Span<T, extent>>
    , public meta::EnableBorrowedContainer<Span<T, extent>> {
public:
    constexpr explicit Span()
    requires(extent == 0)
    = default;

    template<concepts::ContiguousIterator Iter>
    requires(concepts::ConvertibleToNonSlicing<meta::IteratorValue<Iter>, T>)
    constexpr explicit Span(Iter first, types::size_t) : m_data(util::to_address(first)) {
        // DI_ASSERT( count == extent )
    }

    template<concepts::ContiguousIterator Iter, concepts::SizedSentinelFor<Iter> Sent>
    requires(concepts::ConvertibleToNonSlicing<meta::IteratorValue<Iter>, T> && !concepts::ConvertibleTo<Sent, types::size_t>)
    constexpr explicit Span(Iter it, Sent sent) : m_data(util::to_address(it)) {
        // DI_ASSERT( sent - it == extent );
    }

    template<types::size_t size>
    requires(size == extent)
    constexpr Span(T (&array)[size]) : m_data(array) {}

    template<concepts::ConvertibleToNonSlicing<T> U, types::size_t size>
    requires(size == extent)
    constexpr Span(vocab::array::Array<U, size>& array) : m_data(array.data()) {}

    template<typename U, types::size_t size>
    requires(size == extent && concepts::ConvertibleToNonSlicing<U const, T>)
    constexpr Span(vocab::array::Array<U, size> const& array) : m_data(array.data()) {}

    template<concepts::ContiguousContainer Con>
    requires(concepts::SizedContainer<Con> && (concepts::BorrowedContainer<Con> || concepts::Const<T>) && !concepts::Span<Con> &&
             !concepts::Array<Con> && !concepts::LanguageArray<meta::RemoveCVRef<Con>> &&
             concepts::ConvertibleToNonSlicing<meta::ContainerValue<Con>, T>)
    constexpr explicit Span(Con&& container) : m_data(container::data(container)) {
        // DI_ASSERT( container::size(container) == extent )
    }

    template<concepts::ConvertibleToNonSlicing<T> U, types::size_t other_extent>
    requires((other_extent == dynamic_extent || extent == other_extent))
    constexpr explicit(other_extent == dynamic_extent) Span(Span<U, other_extent> const& other) : m_data(other.data()) {
        // DI_ASSERT( other.size() == extent )
    }

    constexpr Span(Span const&) = default;

    constexpr Span& operator=(Span const&) = default;

    constexpr T* begin() const { return m_data; }
    constexpr T* end() const { return m_data + extent; }

    constexpr T& front() const
    requires(extent > 0)
    {
        return *begin();
    }

    constexpr T& back() const
    requires(extent > 0)
    {
        return *(end() - 1);
    }

    constexpr vocab::Optional<T&> at(types::size_t index) const {
        if (index >= extent) {
            return vocab::nullopt;
        }
        return (*this)[index];
    }

    constexpr T& operator[](types::size_t index) const
    requires(extent > 0)
    {
        // DI_ASSSERT( index < extent )
        return begin()[index];
    }

    constexpr T* data() const { return begin(); }

    constexpr types::size_t size() const { return extent; }
    constexpr types::size_t size_in_bytes() const { return sizeof(T) * extent; }

    [[nodiscard]] constexpr bool empty() const { return extent == 0; }

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
            return Span<T, extent - offset> { begin(), end() };
        } else {
            return Span<T, count> { begin(), count };
        }
    }

private:
    T* m_data { nullptr };
};

template<typename T, types::size_t size>
Span(T (&)[size]) -> Span<T, size>;

template<typename T, types::size_t size>
Span(vocab::array::Array<T, size>&) -> Span<T, size>;

template<typename T, types::size_t size>
Span(vocab::array::Array<T, size> const&) -> Span<T const, size>;

// template<typename Iter, typename SentOrSize>
// Span(Iter, SentOrSize) -> Span<meta::RemoveReference<meta::IteratorValue<Iter>>>;

// template<typename Con>
// Span(Con&&) -> Span<meta::RemoveReference<meta::ContainerValue<Con>>>;
}
