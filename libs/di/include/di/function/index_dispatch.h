#pragma once

#include "di/assert/assert_bool.h"
#include "di/function/invoke.h"
#include "di/meta/algorithm.h"
#include "di/meta/language.h"
#include "di/types/prelude.h"
#include "di/util/forward.h"
#include "di/vocab/array/prelude.h"

namespace di::function {
namespace detail {
    template<typename R, usize index, typename F, typename... Args>
    struct IndexDispatchImpl {
        constexpr static auto do_call(F&& function, Args&&... args) -> R {
            if constexpr (concepts::LanguageVoid<R>) {
                function::invoke(di::forward<F>(function), c_<index>, di::forward<Args>(args)...);
            } else {
                return function::invoke(di::forward<F>(function), c_<index>, di::forward<Args>(args)...);
            }
        }
    };

    template<typename R, usize max_index>
    struct IndexDispatch {
        template<typename F, typename... Args>
        constexpr static auto operator()(usize index, F&& function, Args&&... args) -> R {
            auto function_table = []<usize... indices>(meta::ListV<indices...>) {
                return Array<R (*)(F&&, Args&&...), max_index> { (
                    IndexDispatchImpl<R, indices, F, Args...>::do_call)... };
            }(meta::MakeIndexSequence<max_index> {});

            DI_ASSERT(index < max_index);
            return function_table[index](di::forward<F>(function), di::forward<Args>(args)...);
        }
    };
}

template<typename R, usize max_index>
requires(max_index > 0)
constexpr inline auto index_dispatch = detail::IndexDispatch<R, max_index> {};
}
