#pragma once

#include <di/container/algorithm/copy.h>
#include <di/container/algorithm/min.h>
#include <di/container/concepts/container_of.h>
#include <di/container/concepts/input_container.h>
#include <di/container/interface/begin.h>
#include <di/container/meta/container_iterator.h>
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
template<concepts::InputContainer T = container::Vector<byte>>
requires(concepts::ContainerOf<T, byte>)
class VectorReader {
private:
    template<typename U = void>
    using Result = vocab::Result<U>;

    using It = meta::ContainerIterator<T>;
    using Sent = meta::ContainerSentinel<T>;

public:
    VectorReader()
    requires(concepts::DefaultConstructible<T> && concepts::DefaultConstructible<It> &&
             concepts::DefaultConstructible<Sent>)
    = default;

    constexpr explicit VectorReader(T&& container)
        : m_container(di::forward<T>(container)), m_iterator(begin(m_container)), m_sentinel(end(m_container)) {}

    template<typename... Args>
    requires(ConstructibleFrom<T, Args...>)
    constexpr explicit VectorReader(InPlace, Args&&... args)
        : m_container(di::forward<Args>(args)...), m_iterator(begin(m_container)), m_sentinel(end(m_container)) {}

    constexpr auto container() & -> T& { return m_container; }
    constexpr auto container() const& -> T const& { return m_container; }
    constexpr auto container() && -> T&& { return di::move(*this).m_container; }

    constexpr auto read_some(Span<byte> bytes) -> Result<usize> {
        auto* bytes_it = bytes.data();
        auto max_to_read = bytes.size();
        auto nread = 0ZU;
        for (; nread < max_to_read && m_iterator != m_sentinel; ++m_iterator, ++nread) {
            *bytes_it++ = *m_iterator;
        }
        return nread;
    }

private:
    T m_container {};
    It m_iterator {};
    Sent m_sentinel {};
};

template<typename T>
VectorReader(T&&) -> VectorReader<T>;
}

namespace di {
using io::VectorReader;
}
