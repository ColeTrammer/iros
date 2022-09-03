#pragma once

#include <di/types/size_t.h>

namespace di::util {
template<typename T, T... values>
struct IntegerSequence {
    using Value = T;

    constexpr static auto size() { return sizeof...(values); }
};

template<types::size_t... values>
using IndexSequence = IntegerSequence<types::size_t, values...>;

template<typename T, types::size_t count>
using make_integer_sequence = IntegerSequence<T>;

template<types::size_t count>
using make_index_sequence = make_integer_sequence<types::size_t, count>;

template<typename... Types>
using IndexSequenceFor = make_index_sequence<sizeof...(T)>;
}
