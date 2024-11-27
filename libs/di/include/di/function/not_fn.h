#pragma once

#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/in_place.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function {
namespace detail {
    template<typename F>
    class NotFnFunction : public pipeline::EnablePipeline {
    private:
        F m_function;

    public:
        template<typename Fn>
        constexpr NotFnFunction(types::InPlace, Fn&& function) : m_function(util::forward<Fn>(function)) {}

        constexpr NotFnFunction(NotFnFunction const&) = default;
        constexpr NotFnFunction(NotFnFunction&&) = default;

        constexpr auto operator=(NotFnFunction const&) -> NotFnFunction& = delete;
        constexpr auto operator=(NotFnFunction&&) -> NotFnFunction& = delete;

        template<typename... Args>
        constexpr auto operator()(Args&&... args) & -> decltype(!function::invoke(m_function,
                                                                                  util::forward<Args>(args)...)) {
            return !function::invoke(m_function, util::forward<Args>(args)...);
        }

        template<typename... Args>
        constexpr auto operator()(Args&&... args) const& -> decltype(!function::invoke(m_function,
                                                                                       util::forward<Args>(args)...)) {
            return !function::invoke(m_function, util::forward<Args>(args)...);
        }

        template<typename... Args>
        constexpr auto operator()(Args&&... args) && -> decltype(!function::invoke(util::move(m_function),
                                                                                   util::forward<Args>(args)...)) {
            return !function::invoke(util::move(m_function), util::forward<Args>(args)...);
        }

        template<typename... Args>
        constexpr auto operator()(Args&&... args) const&& -> decltype(!function::invoke(util::move(m_function),
                                                                                        util::forward<Args>(args)...)) {
            return !function::invoke(util::move(m_function), util::forward<Args>(args)...);
        }
    };
}

template<concepts::DecayConstructible F>
constexpr auto not_fn(F&& function) {
    return detail::NotFnFunction<meta::Decay<F>>(types::in_place, util::forward<F>(function));
}
}

namespace di {
using function::not_fn;
}
