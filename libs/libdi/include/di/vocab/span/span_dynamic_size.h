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
#include <di/meta/remove_cvref.h>
#include <di/meta/remove_reference.h>
#include <di/types/size_t.h>
#include <di/util/to_address.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/span/span_forward_declaration.h>

namespace di::vocab {
template<typename T>
class Span<T, dynamic_extent>
    : public meta::EnableView<Span<T, dynamic_extent>>
    , public meta::EnableBorrowedContainer<Span<T, dynamic_extent>> {
public:
    constexpr Span() = default;

    template<concepts::ContiguousIterator Iter>
    requires(concepts::ConvertibleToNonSlicing<meta::IteratorValue<Iter>, T>)
    constexpr Span(Iter iterator, types::size_t size) : m_data(util::to_address(iterator)), m_size(size) {}

    template<concepts::ContiguousIterator Iter, concepts::SizedSentinelFor<Iter> Sent>
    requires(concepts::ConvertibleToNonSlicing<meta::IteratorValue<Iter>, T> && !concepts::ConvertibleTo<Sent, types::size_t>)
    constexpr Span(Iter iterator, Sent sentinel) : m_data(util::to_address(iterator)), m_size(sentinel - iterator) {}

    template<types::size_t size>
    constexpr Span(T (&array)[size]) : m_data(array), m_size(size) {}

    template<concepts::ConvertibleToNonSlicing<T> U, types::size_t size>
    constexpr Span(vocab::Array<U, size>& array) : m_data(array.data()), m_size(size) {}

    template<typename U, types::size_t size>
    requires(concepts::ConvertibleToNonSlicing<U const, T>)
    constexpr Span(vocab::Array<U, size> const& array) : m_data(array.data()), m_size(size) {}

    template<concepts::ContiguousContainer Con>
    requires(concepts::SizedContainer<Con> && (concepts::BorrowedContainer<Con> || concepts::Const<T>) && !concepts::Span<Con> &&
             !concepts::Array<Con> && !concepts::LanguageArray<meta::RemoveCVRef<Con>> &&
             concepts::ConvertibleToNonSlicing<meta::ContainerValue<Con>, T>)
    constexpr explicit Span(Con&& container) : m_data(container::data(container)), m_size(container::size(container)) {}

    template<concepts::ConvertibleToNonSlicing<T> U, types::size_t other_extent>
    constexpr Span(Span<U, other_extent> const& other) : m_data(other.data()), m_size(other.size()) {}

    constexpr Span(Span const&) = default;

    constexpr Span& operator=(Span const&) = default;

    constexpr T* begin() const { return data(); }
    constexpr T* end() const { return data() + size(); }

    constexpr Optional<T&> front() const {
        if (empty()) {
            return nullopt;
        }
        return (*this)[0];
    }

    constexpr Optional<T&> back() const {
        if (empty()) {
            return nullopt;
        }
        return (*this)[size() - 1];
    }

    constexpr vocab::Optional<T&> at(types::size_t index) const {
        if (index >= size()) {
            return nullopt;
        }
        return (*this)[index];
    }

    constexpr T& operator[](types::size_t index) const {
        // DI_ASSERT( index < size() )
        return data()[index];
    }

    constexpr T* data() const { return m_data; }

    constexpr types::size_t size() const { return m_size; }
    constexpr types::size_t size_bytes() const { return sizeof(T) * m_size; }
    [[nodiscard]] constexpr bool empty() const { return size() == 0; }

    constexpr Optional<Span> first(types::size_t count) const {
        if (count > size()) {
            return nullopt;
        }
        return Span { data(), count };
    }

    constexpr Optional<Span> last(types::size_t count) const {
        if (count > size()) {
            return nullopt;
        }
        return Span { end() - count, count };
    }

    constexpr Optional<Span> subspan(types::size_t offset) const {
        if (offset > size()) {
            return nullopt;
        }
        return Span { data() + offset, size() - offset };
    }

    constexpr Optional<Span> subspan(types::size_t offset, types::size_t count) const {
        if (offset + count > size()) {
            return nullopt;
        }
        return Span { data() + offset, count };
    }

    template<types::size_t count>
    constexpr Optional<Span<T, count>> first() const {
        if (count > size()) {
            return nullopt;
        }
        return Span<T, count> { data(), count };
    }

    template<types::size_t count>
    constexpr Optional<Span<T, count>> last() const {
        if (count > size()) {
            return nullopt;
        }
        return Span<T, count> { end() - count, count };
    }

    template<types::size_t offset, types::size_t count = dynamic_extent>
    constexpr Optional<Span<T, count>> subspan() const {
        if constexpr (count == dynamic_extent) {
            if (offset > size()) {
                return nullopt;
            }
            return Span { data() + offset, end() };
        } else {
            if (offset + count > size()) {
                return nullopt;
            }
            return Span<T, count> { data() + offset, count };
        }
    }

private:
    T* m_data { nullptr };
    types::size_t m_size { 0 };
};

template<typename Iter, typename SentOrSize>
Span(Iter, SentOrSize) -> Span<meta::RemoveReference<meta::IteratorValue<Iter>>>;

template<typename Con>
Span(Con&&) -> Span<meta::RemoveReference<meta::ContainerValue<Con>>>;
}
