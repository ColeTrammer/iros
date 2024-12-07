#pragma once

#include "di/function/monad/monad_bind.h"
#include "di/function/monad/monad_concept.h"
#include "di/function/monad/monad_fail.h"
#include "di/function/monad/monad_fmap.h"
#include "di/function/monad/monad_fmap_right.h"
#include "di/util/forward.h"

namespace di::function::monad {
template<typename M, typename F>
constexpr auto operator%(M&& m, F&& f) -> decltype(fmap(util::forward<M>(m), util::forward<F>(f))) {
    return fmap(util::forward<M>(m), util::forward<F>(f));
}

template<typename M, typename F>
constexpr auto operator>>(M&& m, F&& f) -> decltype(bind(util::forward<M>(m), util::forward<F>(f))) {
    return bind(util::forward<M>(m), util::forward<F>(f));
}

template<typename M, typename F>
constexpr auto operator<<(M&& m, F&& f) -> decltype(fail(util::forward<M>(m), util::forward<F>(f))) {
    return fail(util::forward<M>(m), util::forward<F>(f));
}

template<typename M, typename F>
constexpr auto operator&(M&& m, F&& f) -> decltype(fmap_right(util::forward<M>(m), util::forward<F>(f))) {
    return fmap_right(util::forward<M>(m), util::forward<F>(f));
}
}
