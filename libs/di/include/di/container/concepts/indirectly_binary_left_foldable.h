#pragma once

#include "di/container/concepts/indirectly_readable.h"
#include "di/container/meta/iterator_reference.h"
#include "di/function/invoke.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"

namespace di::concepts {
namespace detail {
    template<typename F, typename T, typename Iter, typename R>
    concept IndirectlyBinaryLeftFoldableHelper =
        Movable<T> && Movable<R> && ConvertibleTo<T, R> && Invocable<F&, R, meta::IteratorReference<Iter>> &&
        AssignableFrom<R&, meta::InvokeResult<F&, R, meta::IteratorReference<Iter>>>;
}

template<typename F, typename T, typename Iter>
concept IndirectlyBinaryLeftFoldable =
    CopyConstructible<F> && IndirectlyReadable<Iter> && Invocable<F&, T, meta::IteratorReference<Iter>> &&
    DecayConvertible<meta::InvokeResult<F&, T, meta::IteratorReference<Iter>>> &&
    detail::IndirectlyBinaryLeftFoldableHelper<F, T, Iter,
                                               meta::Decay<meta::InvokeResult<F&, T, meta::IteratorReference<Iter>>>>;
}
