#pragma once

#include <di/function/invoke.h>
#include <di/function/monad/monad_bind.h>
#include <di/function/monad/monad_enable.h>
#include <di/function/monad/monad_fail.h>
#include <di/function/monad/monad_fmap.h>
#include <di/function/monad/monad_fmap_right.h>
#include <di/meta/util.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function::monad {
template<typename Self>
class MonadInterface {
public:
    template<typename F>
    requires(concepts::Invocable<decltype(fmap), Self&, F>)
    constexpr auto transform(F&& function) & -> decltype(auto) {
        return fmap(static_cast<Self&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap), Self const&, F>)
    constexpr auto transform(F&& function) const& -> decltype(auto) {
        return fmap(static_cast<Self const&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap), Self &&, F>)
    constexpr auto transform(F&& function) && -> decltype(auto) {
        return fmap(static_cast<Self&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap), Self const &&, F>)
    constexpr auto transform(F&& function) const&& -> decltype(auto) {
        return fmap(static_cast<Self const&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(bind), Self&, F>)
    constexpr auto and_then(F&& function) & -> decltype(auto) {
        return bind(static_cast<Self&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(bind), Self const&, F>)
    constexpr auto and_then(F&& function) const& -> decltype(auto) {
        return bind(static_cast<Self const&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(bind), Self &&, F>)
    constexpr auto and_then(F&& function) && -> decltype(auto) {
        return bind(static_cast<Self&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(bind), Self const &&, F>)
    constexpr auto and_then(F&& function) const&& -> decltype(auto) {
        return bind(static_cast<Self const&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fail), Self&, F>)
    constexpr auto or_else(F&& function) & -> decltype(auto) {
        return fail(static_cast<Self&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fail), Self const&, F>)
    constexpr auto or_else(F&& function) const& -> decltype(auto) {
        return fail(static_cast<Self const&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fail), Self &&, F>)
    constexpr auto or_else(F&& function) && -> decltype(auto) {
        return fail(static_cast<Self&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fail), Self const &&, F>)
    constexpr auto or_else(F&& function) const&& -> decltype(auto) {
        return fail(static_cast<Self const&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap_right), Self&, F>)
    constexpr auto transform_error(F&& function) & -> decltype(auto) {
        return fmap_right(static_cast<Self&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap_right), Self const&, F>)
    constexpr auto transform_error(F&& function) const& -> decltype(auto) {
        return fmap_right(static_cast<Self const&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap_right), Self &&, F>)
    constexpr auto transform_error(F&& function) && -> decltype(auto) {
        return fmap_right(static_cast<Self&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap_right), Self const &&, F>)
    constexpr auto transform_error(F&& function) const&& -> decltype(auto) {
        return fmap_right(static_cast<Self const&&>(*this), util::forward<F>(function));
    }

private:
    constexpr friend auto tag_invoke(types::Tag<enable_monad>, types::InPlaceType<Self>) -> bool { return true; }
};
}
