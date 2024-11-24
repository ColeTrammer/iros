#pragma once

#include <di/any/concepts/impl.h>
#include <di/any/container/any_shared.h>
#include <di/any/types/method.h>
#include <di/container/allocator/allocator.h>
#include <di/function/monad/monad_try.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/operations.h>
#include <di/meta/vocab.h>
#include <di/platform/custom.h>
#include <di/types/in_place.h>
#include <di/util/exchange.h>
#include <di/vocab/optional/nullopt.h>
#include <di/vocab/optional/optional_forward_declaration.h>
#include <di/vocab/span/span_dynamic_size.h>
#include <di/vocab/span/span_fixed_size.h>

namespace di::vocab::byte_buffer {
struct AsWritableByteSpan {
    using Type = Method<AsWritableByteSpan, Span<byte>(This&)>;

    template<typename T>
    requires(concepts::TagInvocable<AsWritableByteSpan, T&> ||
             requires(T& self) {
                 { self.span() } -> concepts::ConvertibleTo<Span<byte>>;
             })
    constexpr static auto operator()(T& self) -> Span<byte> {
        if constexpr (concepts::TagInvocable<AsWritableByteSpan, T&>) {
            static_assert(concepts::ConvertibleTo<meta::TagInvokeResult<AsWritableByteSpan, T&>, Span<byte>>,
                          "as_writable_byte_span() customizations must return a Span<byte>");
            return tag_invoke(AsWritableByteSpan {}, self);
        } else {
            return self.span();
        }
    }
};

struct AsByteSpan {
    using Type = Method<AsByteSpan, Span<byte const>(This const&)>;

    template<typename T>
    requires(concepts::TagInvocable<AsByteSpan, T const&> ||
             requires(T const& self) {
                 { self.span() } -> concepts::ConvertibleTo<Span<byte const>>;
             })
    constexpr static auto operator()(T const& self) -> Span<byte const> {
        if constexpr (concepts::TagInvocable<AsByteSpan, T const&>) {
            static_assert(concepts::ConvertibleTo<meta::TagInvokeResult<AsByteSpan, T const&>, Span<byte const>>,
                          "as_byte_span() customizations must return a Span<byte const>");
            return tag_invoke(AsByteSpan {}, self);
        } else {
            return self.span();
        }
    }
};
}

namespace di {
constexpr inline auto as_writable_byte_span = vocab::byte_buffer::AsWritableByteSpan {};
constexpr inline auto as_byte_span = vocab::byte_buffer::AsByteSpan {};
}

namespace di::vocab::byte_buffer {
template<concepts::Allocator Alloc = platform::DefaultAllocator>
using ByteStore = AnyShared<meta::List<>, Alloc>;

template<concepts::Allocator Alloc = platform::DefaultAllocator>
class ByteBufferImpl;

template<concepts::Allocator Alloc = platform::DefaultAllocator>
class ExclusiveByteBufferImpl;

template<concepts::Allocator Alloc>
class ByteBufferImpl {
public:
    using Store = ByteStore<Alloc>;
    using ByteBuffer = ByteBufferImpl;

    template<concepts::Impl<meta::List<AsByteSpan>> T>
    requires(!concepts::DerivedFrom<T, ByteBufferImpl> && concepts::FallibleAllocator<Alloc>)
    constexpr static auto create(T&& value) -> meta::LikeExpected<meta::AllocatorResult<Alloc>, ByteBufferImpl> {
        auto data = as_byte_span(value);
        auto store = DI_TRY(Store::create(di::forward<T>(value)));
        return ByteBufferImpl(data, di::move(store));
    }

    ByteBufferImpl() = default;

    template<concepts::Impl<meta::List<AsByteSpan>> T>
    requires(!concepts::DerivedFrom<T, ByteBufferImpl> && !concepts::FallibleAllocator<Alloc>)
    constexpr explicit ByteBufferImpl(T&& value) : m_data(as_byte_span(value)), m_store(di::forward<T>(value)) {}

    constexpr explicit ByteBufferImpl(Span<byte const> data, Store&& store) : m_data(data), m_store(di::move(store)) {}
    constexpr explicit ByteBufferImpl(Span<byte const> data, Store const& store) : m_data(data), m_store(store) {}

    constexpr auto data() const -> byte const* { return m_data.data(); }
    constexpr auto size() const -> usize { return m_data.size(); }
    constexpr auto span() const -> Span<byte const> { return m_data; }
    constexpr auto empty() const -> bool { return m_data.empty(); }

    constexpr auto store() const& -> Store const& { return m_store; }

    constexpr auto first(usize count) const -> Optional<ByteBufferImpl> {
        return span().first(count) % [&](Span<byte const> data) {
            return ByteBufferImpl(data, m_store);
        };
    }

    constexpr auto last(usize count) const -> Optional<ByteBufferImpl> {
        return span().last(count) % [&](Span<byte const> data) {
            return ByteBufferImpl(data, m_store);
        };
    }

    constexpr auto slice(usize offset) const -> Optional<ByteBufferImpl> {
        return span().subspan(offset) % [&](Span<byte const> data) {
            return ByteBufferImpl(data, m_store);
        };
    }

    constexpr auto slice(usize offset, usize count) const -> Optional<ByteBufferImpl> {
        return span().subspan(offset, count) % [&](Span<byte const> data) {
            return ByteBufferImpl(data, m_store);
        };
    }

private:
    Span<byte const> m_data;
    Store m_store;
};

template<concepts::Allocator Alloc>
class ExclusiveByteBufferImpl {
public:
    using Store = ByteStore<Alloc>;
    using ByteBuffer = ByteBufferImpl<Alloc>;

    template<concepts::Impl<meta::List<AsWritableByteSpan>> T>
    requires(!concepts::DerivedFrom<T, ExclusiveByteBufferImpl> && concepts::FallibleAllocator<Alloc>)
    constexpr static auto create(T&& value)
        -> meta::LikeExpected<meta::AllocatorResult<Alloc>, ExclusiveByteBufferImpl> {
        auto data = as_writable_byte_span(value);
        auto store = DI_TRY(Store::create(di::forward<T>(value)));
        return ExclusiveByteBufferImpl(data, di::move(store));
    }

    ExclusiveByteBufferImpl() = default;

    template<concepts::Impl<meta::List<AsWritableByteSpan>> T>
    requires(!concepts::DerivedFrom<T, ExclusiveByteBufferImpl> && !concepts::FallibleAllocator<Alloc>)
    constexpr explicit ExclusiveByteBufferImpl(T&& value)
        : m_data(as_writable_byte_span(value)), m_store(di::forward<T>(value)) {}

    constexpr explicit ExclusiveByteBufferImpl(Span<byte> data, Store&& store)
        : m_data(data), m_store(di::move(store)) {}

    constexpr ExclusiveByteBufferImpl(ExclusiveByteBufferImpl const&) = delete;
    constexpr ExclusiveByteBufferImpl(ExclusiveByteBufferImpl&& other) : m_store(di::move(other.m_store)) {
        m_data = di::exchange(other.m_data, Span<byte> {});
    }

    constexpr ExclusiveByteBufferImpl& operator=(ExclusiveByteBufferImpl const&) = delete;
    constexpr ExclusiveByteBufferImpl& operator=(ExclusiveByteBufferImpl&& other) {
        m_store = di::move(other.m_store);
        m_data = di::exchange(other.m_data, Span<byte> {});
    }

    constexpr auto data() const -> byte* { return m_data.data(); }
    constexpr auto size() const -> usize { return m_data.size(); }
    constexpr auto span() const -> Span<byte> { return m_data; }
    constexpr auto empty() const -> bool { return m_data.empty(); }

    constexpr auto store() && -> Store&& {
        m_data = {};
        return di::move(*this).m_store;
    }

    constexpr auto shrink_to_first(usize count) { m_data = m_data.first(count).value_or(Span<byte> {}); }
    constexpr auto shrink_to_last(usize count) { m_data = m_data.last(count).value_or(Span<byte> {}); }
    constexpr auto shrink_to_slice(usize offset) { m_data = m_data.subspan(offset).value_or(Span<byte> {}); }
    constexpr auto shrink_to_slice(usize offset, usize count) {
        m_data = m_data.subspan(offset, count).value_or(Span<byte> {});
    }

    constexpr auto share() && -> ByteBuffer {
        auto data = di::exchange(m_data, Span<byte> {});
        return ByteBuffer(data, di::move(m_store));
    }

    constexpr operator ByteBuffer() && { return share(); }

private:
    Span<byte> m_data;
    Store m_store;
};
}

namespace di {
using ByteBuffer = vocab::byte_buffer::ByteBufferImpl<>;
using ExclusiveByteBuffer = vocab::byte_buffer::ExclusiveByteBufferImpl<>;
}
