#pragma once

#include <liim/container/allocator.h>
#include <liim/container/strings/encoding.h>
#include <liim/container/strings/mutable_string_interface.h>
#include <liim/container/strings/string_view.h>
#include <liim/endian.h>

namespace LIIM::Container::Strings {
template<Encoding Enc, AllocatorOf<EncodingCodeUnit<Enc>> Alloc = StandardAllocator<EncodingCodeUnit<Enc>>>
class SsoStringImpl : public MutableStringInterface<SsoStringImpl<Enc, Alloc>, Enc> {
public:
    using CodeUnit = EncodingCodeUnit<Enc>;
    using CodePoint = EncodingCodePoint<Enc>;
    using Iterator = EncodingIterator<Enc>;

    using VectorValue = CodeUnit;

private:
    // Heap storage is the 3 typical components of a Vector. However, the capacity is
    // assumed the heap capacity is always assumed to be aligned to an even number.
    // The string data is storaged in the inline buffer when the capacity is even, and
    // is stored in the heap buffer when the capacity is odd. This enables the empty
    // string to be represented by all zeros.
    struct HeapStorage {
        LittleEndian<size_t> m_capacity { 0 };
        size_t m_size { 0 };
        CodeUnit* m_data { nullptr };
    };

    constexpr static size_t inline_capacity = (sizeof(HeapStorage) - 1) / sizeof(CodeUnit);

    struct InlineStorage {
        union {
            uint8_t m_size_and_flag { 0 };
            CodeUnit m_padding1;
        };
        union {
            CodeUnit m_inline_data[inline_capacity];
            char m_padding2[sizeof(HeapStorage) - 1];
        };
    };

public:
    constexpr static SsoStringImpl create(StringViewImpl<Enc> view) {
        auto string = SsoStringImpl();
        string.append(view);
        return string;
    }

    constexpr SsoStringImpl() : m_heap_storage() {}

    constexpr SsoStringImpl(SsoStringImpl&& other) : m_heap_storage(other.m_heap_storage) { other.m_heap_storage = HeapStorage(); }

    constexpr SsoStringImpl& operator=(SsoStringImpl&& other) {
        m_heap_storage = other.m_heap_storage;
        other.m_heap_storage = HeapStorage();
    }

    constexpr ~SsoStringImpl() {
        if (!is_small() && span().data()) {
            Alloc().deallocate(span().data(), capacity());
        }
    }

    constexpr Span<CodeUnit const> span() const {
        if (is_small()) {
            return { m_inline_storage.m_inline_data, static_cast<size_t>(m_inline_storage.m_size_and_flag >> 1) };
        } else {
            return { m_heap_storage.m_data, m_heap_storage.m_size };
        }
    }
    constexpr Span<CodeUnit> span() {
        auto result = const_cast<SsoStringImpl const&>(*this).span();
        return { const_cast<CodeUnit*>(result.data()), result.size() };
    }

    constexpr size_t capacity() const {
        if (is_small()) {
            return inline_capacity;
        } else {
            return m_heap_storage.m_capacity & ~1;
        }
    }

    constexpr void reserve(size_t size) {
        if (size <= capacity()) {
            return;
        }

        auto [new_data, new_capacity] = Alloc().allocate(((size + 1) / 2) * 2);
        for (size_t i = 0; i < span().size(); i++) {
            new_data[i] = span()[i];
        }

        if (!is_small() && span().data()) {
            Alloc().deallocate(span().data(), capacity());
        }
        auto old_size = span().size();
        m_heap_storage.m_capacity = new_capacity | 1;
        m_heap_storage.m_size = old_size;
        m_heap_storage.m_data = new_data;
    }

    constexpr void assume_size(size_t size) {
        if (is_small()) {
            assert(size <= inline_capacity);
            m_inline_storage.m_size_and_flag = size << 1;
        } else {
            m_heap_storage.m_size = size;
        }
    }

private:
    constexpr bool is_small() const {
        if
            consteval {
                return false;
            }
        else {
            return (m_inline_storage.m_size_and_flag & 1) == 0;
        }
    }

    union {
        HeapStorage m_heap_storage;
        InlineStorage m_inline_storage;
    };
};
}
