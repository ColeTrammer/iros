#pragma once

#include <di/meta/integer_sequence.h>
#include <di/meta/type_constant.h>
#include <di/types/size_t.h>

namespace di::meta {
// To produce an integer sequence using a minimal number of template
// instantiations, we partition the problem into halves, and construct
// the correcct sequence using the Concat helper.
namespace detail {
    template<typename T, typename X, typename Y>
    struct MakeIntegerSequenceConcatHelper;

    template<typename T, T... s1, T... s2>
    struct MakeIntegerSequenceConcatHelper<T, IntegerSequence<T, s1...>, IntegerSequence<T, s2...>> {
        using Type = IntegerSequence<T, s1..., (sizeof...(s1) + s2)...>;
    };

    template<typename T, types::size_t count>
    struct MakeIntegerSequenceHelper {
        using A = MakeIntegerSequenceHelper<T, count / 2>::Type;
        using B = MakeIntegerSequenceHelper<T, count - count / 2>::Type;
        using Type = MakeIntegerSequenceConcatHelper<T, A, B>::Type;
    };

    template<typename T>
    struct MakeIntegerSequenceHelper<T, 1> : TypeConstant<IntegerSequence<T, 0>> {};

    template<typename T>
    struct MakeIntegerSequenceHelper<T, 0> : TypeConstant<IntegerSequence<T>> {};
}

template<typename T, types::size_t count>
using MakeIntegerSequence = detail::MakeIntegerSequenceHelper<T, count>::Type;
}
