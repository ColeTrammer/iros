#pragma once

#include <di/concepts/copyable.h>
#include <di/container/vector/span_fixed_size.h>
#include <di/types/size_t.h>
#include <di/util/swap.h>
#include <di/vocab/optional.h>

namespace di::vocab::array {
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
        // DI_ASSERT( index < extent )
        return begin()[index];
    }
    constexpr T const& operator[](types::size_t index) const {
        // DI_ASSERT( index < extent )
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

    constexpr container::Span<T, extent> span() { return container::Span { *this }; }
    constexpr container::Span<T const, extent> span() const { return container::Span { *this }; }

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

    template<types::size_t offset, types::size_t count = container::dynamic_extent>
    requires(offset <= extent && (count == container::dynamic_extent || offset + count <= extent))
    constexpr auto subspan() {
        return this->span().template subspan<offset, count>();
    }

    template<types::size_t offset, types::size_t count = container::dynamic_extent>
    requires(offset <= extent && (count == container::dynamic_extent || offset + count <= extent))
    constexpr auto subspan() const {
        return this->span().template subspan<offset, count>();
    }

private:
    constexpr friend void tag_invoke(types::Tag<util::swap>, Array& a, Array& b)
    requires(concepts::Swappable<T>)
    {
        for (types::size_t i = 0; i < extent; i++) {
            util::swap(a[i], b[i]);
        }
    }
};

template<typename T, typename... U>
Array(T, U...) -> Array<T, 1 + sizeof...(U)>;
}
