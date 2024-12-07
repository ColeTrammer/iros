#pragma once

#include "di/container/concepts/borrowed_container.h"
#include "di/container/concepts/contiguous_container.h"
#include "di/container/concepts/contiguous_iterator.h"
#include "di/container/concepts/sized_container.h"
#include "di/container/concepts/sized_sentinel_for.h"
#include "di/container/interface/data.h"
#include "di/container/interface/size.h"
#include "di/container/meta/container_reference.h"
#include "di/container/meta/enable_borrowed_container.h"
#include "di/container/meta/enable_view.h"
#include "di/container/meta/iterator_reference.h"
#include "di/container/vector/constant_vector_interface.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/meta/trivial.h"
#include "di/meta/vocab.h"
#include "di/types/size_t.h"
#include "di/util/to_address.h"
#include "di/vocab/optional/prelude.h"
#include "di/vocab/span/span_forward_declaration.h"

namespace di::vocab {
template<typename T>
class Span<T, dynamic_extent>
    : public meta::EnableView<Span<T, dynamic_extent>>
    , public meta::EnableBorrowedContainer<Span<T, dynamic_extent>>
    , public container::ConstantVectorInterface<Span<T, dynamic_extent>, T> {
public:
    using Value = T;
    using ConstValue = T;

    constexpr Span() = default;

    template<concepts::ContiguousIterator Iter>
    requires(concepts::QualificationConvertibleTo<meta::RemoveReference<meta::IteratorReference<Iter>>, T>)
    constexpr Span(Iter iterator, types::size_t size) : m_data(util::to_address(iterator)), m_size(size) {}

    template<concepts::ContiguousIterator Iter, concepts::SizedSentinelFor<Iter> Sent>
    requires(concepts::QualificationConvertibleTo<meta::RemoveReference<meta::IteratorReference<Iter>>, T> &&
             !concepts::ConvertibleTo<Sent, types::size_t>)
    constexpr Span(Iter iterator, Sent sentinel) : m_data(util::to_address(iterator)), m_size(sentinel - iterator) {}

    template<types::size_t size>
    constexpr Span(T (&array)[size]) : m_data(array), m_size(size) {}

    template<concepts::QualificationConvertibleTo<T> U, types::size_t size>
    constexpr Span(vocab::Array<U, size>& array) : m_data(array.data()), m_size(size) {}

    template<typename U, types::size_t size>
    requires(concepts::QualificationConvertibleTo<U const, T>)
    constexpr Span(vocab::Array<U, size> const& array) : m_data(array.data()), m_size(size) {}

    template<concepts::ContiguousContainer Con>
    requires(concepts::SizedContainer<Con> && (concepts::BorrowedContainer<Con> || concepts::Const<T>) &&
             !concepts::Span<Con> && !concepts::Array<Con> && !concepts::LanguageArray<meta::RemoveCVRef<Con>> &&
             concepts::QualificationConvertibleTo<meta::RemoveReference<meta::ContainerReference<Con>>, T>)
    constexpr explicit Span(Con&& container) : m_data(container::data(container)), m_size(container::size(container)) {}

    template<concepts::QualificationConvertibleTo<T> U, types::size_t other_extent>
    constexpr Span(Span<U, other_extent> const& other) : m_data(other.data()), m_size(other.size()) {}

    constexpr Span(Span const&) = default;

    constexpr auto operator=(Span const&) -> Span& = default;

    constexpr auto data() const -> T* { return m_data; }
    constexpr auto size() const -> types::size_t { return m_size; }

    constexpr auto span() const -> Span { return *this; }

    template<concepts::ImplicitLifetime U>
    requires(concepts::SameAs<Byte, T>)
    auto typed_pointer(size_t byte_offset) const -> Optional<U*> {
        if (byte_offset + sizeof(U) > size()) {
            return nullopt;
        }
        return reinterpret_cast<U*>(m_data + byte_offset);
    }

    template<concepts::ImplicitLifetime U>
    requires(concepts::SameAs<Byte const, T>)
    auto typed_pointer(size_t byte_offset) const -> Optional<U const*> {
        if (byte_offset + sizeof(U) > size()) {
            return nullopt;
        }
        return reinterpret_cast<U const*>(m_data + byte_offset);
    }

    template<concepts::ImplicitLifetime U>
    requires(concepts::SameAs<Byte, T>)
    auto typed_pointer_unchecked(size_t byte_offset) const -> U* {
        return reinterpret_cast<U*>(m_data + byte_offset);
    }

    template<concepts::ImplicitLifetime U>
    requires(concepts::SameAs<Byte const, T>)
    auto typed_pointer_unchecked(size_t byte_offset) const -> U const* {
        return reinterpret_cast<U const*>(m_data + byte_offset);
    }

    template<concepts::ImplicitLifetime U>
    requires(concepts::SameAs<Byte, T>)
    auto typed_span(size_t byte_offset, size_t count) const -> Optional<Span<U>> {
        if (byte_offset + sizeof(U) * count > size()) {
            return nullopt;
        }
        return Span<U> { typed_pointer_unchecked<U>(byte_offset), count };
    }

    template<concepts::ImplicitLifetime U>
    requires(concepts::SameAs<Byte const, T>)
    auto typed_span(size_t byte_offset, size_t count) const -> Optional<Span<U const>> {
        if (byte_offset + sizeof(U) * count > size()) {
            return nullopt;
        }
        return Span<U const> { typed_pointer_unchecked<U const>(byte_offset), count };
    }

    template<concepts::ImplicitLifetime U>
    requires(concepts::SameAs<Byte, T>)
    auto typed_span_unchecked(size_t byte_offset, size_t count) const -> Span<U> {
        return Span<U> { typed_pointer_unchecked<U>(byte_offset), count };
    }

    template<concepts::ImplicitLifetime U>
    requires(concepts::SameAs<Byte const, T>)
    auto typed_span_unchecked(size_t byte_offset, size_t count) const -> Span<U const> {
        return Span<U const> { typed_pointer_unchecked<U const>(byte_offset), count };
    }

private:
    T* m_data { nullptr };
    types::size_t m_size { 0 };
};

template<typename Iter, typename SentOrSize>
Span(Iter, SentOrSize) -> Span<meta::RemoveReference<meta::IteratorReference<Iter>>>;

template<typename Con>
Span(Con&&) -> Span<meta::RemoveReference<meta::ContainerReference<Con>>>;
}
