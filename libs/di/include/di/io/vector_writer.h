#pragma once

#include <di/container/concepts/container_of.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector.h>
#include <di/container/vector/vector_append_container.h>
#include <di/meta/operations.h>
#include <di/types/byte.h>
#include <di/types/in_place.h>
#include <di/util/move.h>
#include <di/vocab/error/result.h>
#include <di/vocab/expected/invoke_as_fallible.h>
#include <di/vocab/expected/try_infallible.h>
#include <di/vocab/span/span_forward_declaration.h>

namespace di::io {
template<concepts::detail::MutableVector T = container::Vector<byte>>
requires(concepts::ContainerOf<T, byte>)
class VectorWriter {
private:
    template<typename U = void>
    using Result = vocab::Result<U>;

public:
    VectorWriter()
    requires(concepts::DefaultConstructible<T>)
    = default;

    template<typename... Args>
    requires(ConstructibleFrom<T, Args...>)
    constexpr explicit VectorWriter(InPlace, Args&&... args) : m_vector(di::forward<Args>(args)...) {}

    constexpr auto vector() & -> T& { return m_vector; }
    constexpr auto vector() const& -> T const& { return m_vector; }
    constexpr auto vector() && -> T&& { return di::move(*this).m_vector; }

    constexpr auto write_some(Span<byte const> bytes) -> Result<usize> {
        return invoke_as_fallible([&] {
                   return container::vector::append_container(vector(), bytes);
               }) % [&] {
            return bytes.size();
        } | try_infallible;
    }
    constexpr auto flush() -> Result<> { return {}; }

private:
    T m_vector;
};
}

namespace di {
using io::VectorWriter;
}
