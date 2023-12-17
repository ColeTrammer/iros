#pragma once

#include <di/function/curry_back.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/types/integers.h>
#include <di/util/declval.h>

namespace di::container::erase_ns {
struct EraseIfFunction;
template<typename Con, typename F>
concept CustomEraseIf = concepts::TagInvocable<EraseIfFunction, Con&, F>;

template<typename Con, typename F>
concept MemberEraseIf = requires(Con& container, F&& function) {
    { container.erase_if(di::forward<F>(function)) } -> SameAs<usize>;
};

struct EraseIfFunction {
    template<typename Con, typename F>
    requires(CustomEraseIf<Con, F> || MemberEraseIf<Con, F>)
    constexpr auto operator()(Con& container, F&& function) const -> usize {
        if constexpr (CustomEraseIf<Con, F>) {
            static_assert(SameAs<usize, meta::TagInvokeResult<EraseIfFunction, Con&, F>>,
                          "Customizations of di::erase_if() must return usize.");
            return tag_invoke(*this, container, di::forward<F>(function));
        } else {
            return container.erase_if(di::forward<F>(function));
        }
    }
};

template<typename Con, typename T>
concept CustomErase = concepts::TagInvocable<EraseIfFunction, Con&, T const&>;

template<typename Con, typename T>
concept MemberErase = requires(Con& container, T const& value) {
    { container.erase(value) } -> SameAs<usize>;
};

template<typename Con, typename T>
concept EraseIfErase = requires(Con& container) {
    {
        EraseIfFunction {}(container, [](auto const& element) {
            return element == di::declval<T const&>();
        })
    } -> SameAs<usize>;
};

struct EraseFunction {
    template<typename Con, typename T>
    requires(CustomErase<Con, T> || MemberErase<Con, T> || EraseIfErase<Con, T>)
    constexpr auto operator()(Con& container, T const& value) const -> usize {
        if constexpr (CustomErase<Con, T>) {
            static_assert(SameAs<usize, meta::TagInvokeResult<EraseIfFunction, Con&, T const&>>,
                          "Customizations of di::erase() must return usize.");
            return tag_invoke(*this, container, value);
        } else if constexpr (MemberErase<Con, T>) {
            return container.erase(value);
        } else {
            return EraseIfFunction {}(container, [&](auto const& element) {
                return element == value;
            });
        }
    }
};
}

namespace di::container {
constexpr inline auto erase = di::curry_back(erase_ns::EraseFunction {}, c_<2zu>);
constexpr inline auto erase_if = di::curry_back(erase_ns::EraseIfFunction {}, c_<2zu>);
}

namespace di {
using container::erase;
using container::erase_if;
}
