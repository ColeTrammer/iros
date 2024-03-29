#pragma once

#include <di/assert/assert_bool.h>
#include <di/function/invoke.h>
#include <di/meta/algorithm.h>
#include <di/meta/language.h>
#include <di/types/prelude.h>
#include <di/util/forward.h>
#include <di/vocab/array/prelude.h>

namespace di::function {
template<typename R, size_t max_index, typename F, typename... Args>
constexpr R index_dispatch(size_t index, F&& function, Args&&... args) {
    auto function_table = []<size_t... indices>(meta::ListV<indices...>) {
        return Array<R (*)(F&&, Args&&...), max_index> { ([](F&& function, Args&&... args) -> R {
            if constexpr (concepts::LanguageVoid<R>) {
                function::invoke(util::forward<F>(function), c_<indices>, util::forward<Args>(args)...);
            } else {
                return function::invoke(util::forward<F>(function), c_<indices>, util::forward<Args>(args)...);
            }
        })... };
    }(meta::MakeIndexSequence<max_index> {});

    DI_ASSERT(index < max_index);
    return function_table[index](util::forward<F>(function), util::forward<Args>(args)...);
}
}
