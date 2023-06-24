#pragma once

#include <di/container/iterator/iterator_move.h>
#include <di/container/meta/iterator_reference.h>
#include <di/container/meta/iterator_rvalue.h>
#include <di/container/meta/iterator_value.h>
#include <di/meta/common.h>
#include <di/meta/core.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    concept IndirectlyReadableHelper =
        requires(T const input) {
            typename meta::IteratorValue<T>;
            typename meta::IteratorReference<T>;
            typename meta::IteratorRValue<T>;
            { *input } -> SameAs<meta::IteratorReference<T>>;
            { container::iterator_move(input) } -> SameAs<meta::IteratorRValue<T>>;
        } && concepts::CommonReferenceWith<meta::IteratorReference<T>&&, meta::IteratorValue<T>&> &&
        concepts::CommonReferenceWith<meta::IteratorReference<T>&&, meta::IteratorRValue<T>&&> &&
        concepts::CommonReferenceWith<meta::IteratorRValue<T>&&, meta::IteratorValue<T> const&>;
}

template<typename T>
concept IndirectlyReadable = detail::IndirectlyReadableHelper<meta::RemoveCVRef<T>>;
}
