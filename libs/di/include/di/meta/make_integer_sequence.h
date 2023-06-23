#pragma once

#include <di/meta/core.h>
#include <di/types/prelude.h>

namespace di::meta {
// To produce an integer sequence using a minimal number of template
// instantiations, we partition the problem into halves, and construct
// the correcct sequence using the Concat helper.
namespace detail {
    template<typename T, typename X, typename Y>
    struct MakeIntegerSequenceConcatHelper;

    template<typename T, T... s1, T... s2>
    struct MakeIntegerSequenceConcatHelper<T, ListV<s1...>, ListV<s2...>> {
        using Type = ListV<s1..., (T(sizeof...(s1) + s2))...>;
    };

    template<typename T, usize count>
    struct MakeIntegerSequenceHelper {
        using A = MakeIntegerSequenceHelper<T, count / 2>::Type;
        using B = MakeIntegerSequenceHelper<T, count - count / 2>::Type;
        using Type = MakeIntegerSequenceConcatHelper<T, A, B>::Type;
    };

    template<typename T>
    struct MakeIntegerSequenceHelper<T, 1> : TypeConstant<ListV<T(0)>> {};

    template<typename T>
    struct MakeIntegerSequenceHelper<T, 0> : TypeConstant<ListV<>> {};
}

template<typename T, usize count>
using MakeIntegerSequence = detail::MakeIntegerSequenceHelper<T, count>::Type;
}
