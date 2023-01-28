#pragma once

#include <liim/container/strings/encoding.h>
#include <liim/container/strings/read_only_string_interface.h>
#include <liim/container/strings/string_view.h>
#include <liim/container/vector/vector_algorithm.h>

namespace LIIM::Container::Strings {
template<typename Self, Encoding Enc>
class MutableStringInterface : public ReadonlyStringInterface<Self, Enc> {
public:
    using CodeUnit = EncodingCodeUnit<Enc>;
    using CodePoint = EncodingCodePoint<Enc>;
    using Iterator = EncodingIterator<Enc>;

    constexpr Self& self() { return static_cast<Self&>(*this); }

    constexpr auto clear() { return Algorithm::clear(Enc(), self()); }

    constexpr auto append(Span<CodeUnit const> code_units) { return Algorithm::append(Enc(), self(), code_units); }

    constexpr auto push_back(CodePoint code_point) { return Algorithm::push_back(Enc(), self(), code_point); }
    constexpr auto pop_back() { return Algorithm::pop_back(Enc(), self()); }

    constexpr auto erase(Iterator position) { return Algorithm::erase(Enc(), self(), position); }
    constexpr auto erase(Iterator begin, Iterator end) { return Algorithm::erase(Enc(), self(), begin, end); }

    constexpr auto insert(Iterator position, CodePoint code_point) { return Algorithm::insert(Enc(), self(), position, code_point); }
    constexpr auto insert(Iterator position, Span<CodeUnit const> code_units) {
        return Algorithm::insert(Enc(), self(), position, code_units);
    }

    constexpr auto replace(Iterator begin, Iterator end, Span<CodeUnit const> replacement) {
        return Algorithm::replace(Enc(), self(), begin, end, replacement);
    }
};
}
