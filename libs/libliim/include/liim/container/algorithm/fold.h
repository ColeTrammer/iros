#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Algorithm {
struct FoldFunction {
    template<Container C, typename Init, typename Func, typename R = InvokeResult<Func, Init, ContainerValueType<C>>::type>
    requires(SameAs<typename InvokeResult<Func, R, ContainerValueType<C>>::type, R>&& IsConvertible<Init, R>::value&&
                 MoveAssignable<R>) constexpr R
    operator()(C&& container, Init initial_value, Func&& function) const {
        auto result = R(move(initial_value));
        auto start = container.begin();
        auto end = container.end();
        for (auto it = move(start); it != end; ++it) {
            result = invoke(forward<Func>(function), move(result), *it);
        }
        return result;
    }
};

constexpr inline auto fold = FoldFunction {};
}
