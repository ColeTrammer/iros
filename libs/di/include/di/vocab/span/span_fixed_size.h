#pragma once

#include <di/assert/assert_bool.h>
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
#include <di/meta/compare.h>
#include <di/meta/constexpr.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/meta/vocab.h>
#include <di/types/size_t.h>
#include <di/util/add_member_get.h>
#include <di/util/get_in_place.h>
#include <di/util/to_address.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/span/span_forward_declaration.h>
#include <di/vocab/tuple/apply.h>
#include <di/vocab/tuple/enable_generate_structed_bindings.h>
#include <di/vocab/tuple/tuple_element.h>
#include <di/vocab/tuple/tuple_size.h>

namespace di::vocab {
template<typename T, types::size_t extent>
struct Array;

template<typename T, types::size_t extent>
requires(extent != dynamic_extent)
class Span<T, extent>
    : public meta::EnableView<Span<T, extent>>
    , public meta::EnableBorrowedContainer<Span<T, extent>>
    , public util::AddMemberGet<Span<T, extent>> {
public:
    using Element = T;

    constexpr explicit Span()
    requires(extent == 0)
    = default;

    template<concepts::ContiguousIterator Iter>
    requires(concepts::QualificationConvertibleTo<meta::RemoveReference<meta::IteratorReference<Iter>>, T>)
    constexpr explicit Span(Iter first, types::size_t count) : m_data(util::to_address(first)) {
        DI_ASSERT(count == extent);
    }

    template<concepts::ContiguousIterator Iter, concepts::SizedSentinelFor<Iter> Sent>
    requires(concepts::QualificationConvertibleTo<meta::RemoveReference<meta::IteratorReference<Iter>>, T> &&
             !concepts::ConvertibleTo<Sent, types::size_t>)
    constexpr explicit Span(Iter it, Sent sent) : m_data(util::to_address(it)) {
        DI_ASSERT(sent - it == extent);
    }

    template<types::size_t size>
    requires(size == extent)
    constexpr Span(T (&array)[size]) : m_data(array) {}

    template<concepts::QualificationConvertibleTo<T> U, types::size_t size>
    requires(size == extent)
    constexpr Span(vocab::Array<U, size>& array) : m_data(array.data()) {}

    template<typename U, types::size_t size>
    requires(size == extent && concepts::QualificationConvertibleTo<U const, T>)
    constexpr Span(vocab::Array<U, size> const& array) : m_data(array.data()) {}

    template<concepts::ContiguousContainer Con>
    requires(concepts::SizedContainer<Con> && (concepts::BorrowedContainer<Con> || concepts::Const<T>) &&
             !concepts::Span<Con> && !concepts::Array<Con> && !concepts::LanguageArray<meta::RemoveCVRef<Con>> &&
             concepts::QualificationConvertibleTo<meta::RemoveReference<meta::ContainerReference<Con>>, T>)
    constexpr explicit Span(Con&& container) : m_data(container::data(container)) {
        DI_ASSERT(container::size(container) == extent);
    }

    template<concepts::QualificationConvertibleTo<T> U, types::size_t other_extent>
    requires((other_extent == dynamic_extent || extent == other_extent))
    constexpr explicit(other_extent == dynamic_extent) Span(Span<U, other_extent> const& other) : m_data(other.data()) {
        DI_ASSERT(other.size() == extent);
    }

    constexpr Span(Span const&) = default;

    constexpr auto operator=(Span const&) -> Span& = default;

    constexpr auto begin() const -> T* { return data(); }
    constexpr auto end() const -> T* { return data() + extent; }

    constexpr auto front() const -> T&
    requires(extent > 0)
    {
        return *data();
    }

    constexpr auto back() const -> T& requires(extent > 0) { return *(end() - 1); }

    constexpr auto at(types::size_t index) const -> vocab::Optional<T&> {
        if (index >= extent) {
            return nullopt;
        }
        return (*this)[index];
    }

    constexpr auto operator[](types::size_t index) const -> T& requires(extent > 0) {
        DI_ASSERT(index < extent);
        return data()[index];
    }

    constexpr auto data() const -> T* {
        return m_data;
    }

    constexpr auto size() const -> types::size_t { return extent; }
    constexpr auto size_bytes() const -> types::size_t { return sizeof(T) * extent; }

    [[nodiscard]] constexpr auto empty() const -> bool { return extent == 0; }

    constexpr auto first(types::size_t count) const -> Optional<Span<T>> {
        if (count > extent) {
            return nullopt;
        }
        return Span<T> { data(), count };
    }

    constexpr auto last(types::size_t count) const -> Optional<Span<T>> {
        if (count > extent) {
            return nullopt;
        }
        return Span<T> { end() - count, count };
    }

    constexpr auto subspan(types::size_t offset) const -> Optional<Span<T>> {
        if (offset > extent) {
            return nullopt;
        }
        return Span<T> { data() + offset, extent - offset };
    }

    constexpr auto subspan(types::size_t offset, types::size_t count) const -> Optional<Span<T>> {
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

    template<typename U = meta::RemoveCV<T>>
    requires(concepts::CopyConstructible<U>)
    constexpr auto to_owned() const -> Array<U, extent> {
        return apply(
            [](auto const&... args) {
                return Array<U, extent> { args... };
            },
            *this);
    }

private:
    constexpr friend auto operator==(Span a, Span b) -> bool
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
    requires(concepts::QualificationConvertibleTo<It, T*>)
    constexpr friend auto tag_invoke(types::Tag<container::reconstruct>, InPlaceType<Span>, It first, Sent last)
        -> Span<T> {
        return Span<T>(util::move(first), util::move(last));
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr friend auto tag_invoke(types::Tag<tuple_element>, types::InPlaceType<Span>, Constexpr<index>)
        -> InPlaceType<T> {
        return {};
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr friend auto tag_invoke(types::Tag<tuple_element>, types::InPlaceType<Span const>, Constexpr<index>)
        -> InPlaceType<T> {
        return {};
    }

    constexpr friend auto tag_invoke(types::Tag<tuple_size>, types::InPlaceType<Span>) -> types::size_t {
        return extent;
    }

    template<types::size_t index>
    requires(index < extent)
    constexpr friend auto tag_invoke(types::Tag<util::get_in_place>, Constexpr<index>, Span self) -> T& {
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
