#pragma once

#include <liim/container/allocator.h>
#include <liim/container/new_vector.h>
#include <liim/container/strings/encoding.h>
#include <liim/container/strings/mutable_string_interface.h>
#include <liim/container/strings/string_view.h>

namespace LIIM::Container::Strings {
template<Encoding Enc, AllocatorOf<EncodingCodeUnit<Enc>> Alloc = StandardAllocator<EncodingCodeUnit<Enc>>>
class HeapStringImpl : public MutableStringInterface<HeapStringImpl<Enc, Alloc>, Enc> {
public:
    using CodeUnit = EncodingCodeUnit<Enc>;
    using CodePoint = EncodingCodePoint<Enc>;
    using Iterator = EncodingIterator<Enc>;

    using VectorValue = CodeUnit;

    constexpr static auto create(StringViewImpl<Enc> view) {
        return result_and_then(collect_vector(view.span()), [](auto&& code_units) {
            return HeapStringImpl(move(code_units));
        });
    }

    constexpr HeapStringImpl() = default;

    constexpr Span<CodeUnit> span() { return m_code_units.span(); }
    constexpr Span<CodeUnit const> span() const { return m_code_units.span(); }

    constexpr auto capacity() const { return m_code_units.capacity(); }

    constexpr auto reserve(size_t capacity) { return m_code_units.reserve(capacity); }
    constexpr auto assume_size(size_t size) { return m_code_units.assume_size(size); }

private:
    constexpr HeapStringImpl(NewVector<CodeUnit, Alloc>&& code_units) : m_code_units(move(code_units)) {}

    NewVector<CodeUnit, Alloc> m_code_units;
};
}
