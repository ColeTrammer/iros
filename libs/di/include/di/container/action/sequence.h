#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/function/curry_back.h>
#include <di/function/invoke.h>
#include <di/meta/language.h>
#include <di/meta/vocab.h>
#include <di/util/create.h>
#include <di/vocab/expected/prelude.h>

namespace di::container {
namespace detail {
    struct SequenceFunction {
        template<concepts::ViewableContainer Con, typename F, typename T = meta::ContainerReference<Con>,
                 typename R = meta::InvokeResult<F, T>, typename G = meta::LikeExpected<R, void>>
        constexpr G operator()(Con&& container, F function) const {
            auto view = view::all(util::forward<Con>(container));
            for (T value : view) {
                if constexpr (concepts::Expected<R> && !concepts::LanguageVoid<meta::ExpectedError<R>>) {
                    auto result = function::invoke(function, util::forward<T>(value));
                    if (!result) {
                        return G { unexpect, util::move(result).error() };
                    }
                } else {
                    function::invoke_r<void>(function, util::forward<T>(value));
                }
            }
            return util::create<G>();
        }
    };
}

constexpr inline auto sequence = function::curry_back(detail::SequenceFunction {}, meta::c_<2zu>);
}
