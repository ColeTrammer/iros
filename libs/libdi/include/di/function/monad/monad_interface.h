#pragma once

#include <di/concepts/decay_same_as.h>
#include <di/function/invoke.h>
#include <di/function/monad/monad_bind.h>
#include <di/function/monad/monad_enable.h>
#include <di/function/monad/monad_fail.h>
#include <di/function/monad/monad_fmap.h>
#include <di/function/monad/monad_fmap_right.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function::monad {
template<typename Self>
class MonadInterface {
public:
    template<typename F>
    requires(concepts::Invocable<decltype(fmap), Self&, F>)
    constexpr decltype(auto) transform(F&& function) & {
        return fmap(static_cast<Self&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap), Self const&, F>)
    constexpr decltype(auto) transform(F&& function) const& {
        return fmap(static_cast<Self const&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap), Self &&, F>)
    constexpr decltype(auto) transform(F&& function) && {
        return fmap(static_cast<Self&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap), Self const &&, F>)
    constexpr decltype(auto) transform(F&& function) const&& {
        return fmap(static_cast<Self const&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(bind), Self&, F>)
    constexpr decltype(auto) and_then(F&& function) & {
        return bind(static_cast<Self&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(bind), Self const&, F>)
    constexpr decltype(auto) and_then(F&& function) const& {
        return bind(static_cast<Self const&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(bind), Self &&, F>)
    constexpr decltype(auto) and_then(F&& function) && {
        return bind(static_cast<Self&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(bind), Self const &&, F>)
    constexpr decltype(auto) and_then(F&& function) const&& {
        return bind(static_cast<Self const&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fail), Self&, F>)
    constexpr decltype(auto) or_else(F&& function) & {
        return fail(static_cast<Self&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fail), Self const&, F>)
    constexpr decltype(auto) or_else(F&& function) const& {
        return fail(static_cast<Self const&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fail), Self &&, F>)
    constexpr decltype(auto) or_else(F&& function) && {
        return fail(static_cast<Self&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fail), Self const &&, F>)
    constexpr decltype(auto) or_else(F&& function) const&& {
        return fail(static_cast<Self const&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap_right), Self&, F>)
    constexpr decltype(auto) transform_error(F&& function) & {
        return fmap_right(static_cast<Self&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap_right), Self const&, F>)
    constexpr decltype(auto) transform_error(F&& function) const& {
        return fmap_right(static_cast<Self const&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap_right), Self &&, F>)
    constexpr decltype(auto) transform_error(F&& function) && {
        return fmap_right(static_cast<Self&&>(*this), util::forward<F>(function));
    }

    template<typename F>
    requires(concepts::Invocable<decltype(fmap_right), Self const &&, F>)
    constexpr decltype(auto) transform_error(F&& function) const&& {
        return fmap_right(static_cast<Self const&&>(*this), util::forward<F>(function));
    }

private:
    constexpr friend bool tag_invoke(types::Tag<enable_monad>, types::InPlaceType<Self>) { return true; }
};
}
