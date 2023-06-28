#pragma once

#include <di/container/algorithm/copy.h>
#include <di/container/algorithm/min.h>
#include <di/container/concepts/container_of.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector.h>
#include <di/container/vector/vector_append_container.h>
#include <di/container/view/drop.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
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
class VectorReader {
private:
    template<typename U = void>
    using Result = vocab::Result<U>;

public:
    VectorReader()
    requires(concepts::DefaultConstructible<T>)
    = default;

    constexpr explicit VectorReader(T&& vector) : m_vector(di::move(vector)) {}

    template<typename... Args>
    requires(ConstructibleFrom<T, Args...>)
    constexpr explicit VectorReader(InPlace, Args&&... args) : m_vector(di::forward<Args>(args)...) {}

    constexpr auto vector() & -> T& { return m_vector; }
    constexpr auto vector() const& -> T const& { return m_vector; }
    constexpr auto vector() && -> T&& { return di::move(*this).m_vector; }

    constexpr auto read_some(Span<byte> bytes) -> Result<usize> {
        auto to_read = min(bytes.size(), m_vector.size() - m_offset);
        copy(m_vector | drop(m_offset) | take(to_read), bytes.data());
        m_offset += to_read;
        return to_read;
    }

private:
    T m_vector;
    usize m_offset { 0 };
};
}

namespace di {
using io::VectorReader;
}
