#pragma once

#include "di/any/concepts/interface.h"
#include "di/any/meta/method_erased_signature.h"
#include "di/meta/operations.h"
#include "di/util/as_const.h"

namespace di::concepts {
namespace detail {
    template<typename M, typename T>
    concept VTableValidFor = Method<meta::Type<M>> && requires(T const vtable) {
        { vtable[meta::Type<M> {}] } -> SameAs<meta::MethodErasedSignature<meta::Type<M>>*>;
    };
}

template<typename T, typename I>
concept VTableFor = DefaultConstructible<T> && Copyable<T> && Interface<I> && requires(T vtable) {
    vtable.reset();
    { util::as_const(vtable).empty() } -> BooleanTestable;
} && requires(I* interface) { []<detail::VTableValidFor<T>... Methods>(meta::List<Methods...>*) {}(interface); };
}
