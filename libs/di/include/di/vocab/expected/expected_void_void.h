#pragma once

#include "di/function/monad/monad_interface.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/types/in_place.h"
#include "di/util/unreachable.h"
#include "di/vocab/expected/expected_forward_declaration.h"

namespace di::vocab {
template<>
class [[nodiscard]] Expected<void, void> : public function::monad::MonadInterface<Expected<void, void>> {
public:
    using Value = void;
    using Error = void;

    constexpr Expected() = default;
    constexpr Expected(Expected const&) = default;
    constexpr Expected(Expected&) = default;

    constexpr explicit Expected(types::InPlace) {}

    constexpr ~Expected() = default;

    constexpr auto operator=(Expected const&) -> Expected& = default;
    constexpr auto operator=(Expected&&) -> Expected& = default;

    constexpr explicit operator bool() const { return true; }
    constexpr auto has_value() const -> bool { return true; }

    constexpr void operator*() const& {}
    constexpr void operator*() && {}

    constexpr void value() const& {}
    constexpr void value() && {}

    constexpr void emplace() {}

    auto __try_did_fail() && -> Expected { util::unreachable(); }
    constexpr auto __try_did_succeed() && -> Expected { return Expected {}; }
    constexpr void __try_move_out() && {}

private:
    template<typename G>
    constexpr friend auto operator==(Expected const&, Expected<void, G> const& b) -> bool {
        return b.has_value();
    }

    template<typename G>
    constexpr friend auto operator==(Expected const&, Unexpected<G> const&) -> bool {
        return false;
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename U = meta::UnwrapRefDecay<meta::InvokeResult<F>>>
    constexpr friend auto tag_invoke(types::Tag<function::monad::fmap>, Self&&, F&& function) -> Expected<U, void> {
        if constexpr (concepts::LanguageVoid<U>) {
            function::invoke(util::forward<F>(function));
            return {};
        } else {
            return function::invoke(util::forward<F>(function));
        }
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F, typename R = meta::InvokeResult<F>>
    requires(concepts::Expected<R>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::bind>, Self&&, F&& function) -> R {
        return function::invoke(util::forward<F>(function));
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F>
    requires(concepts::ConstructibleFrom<Expected, Self>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::fail>, Self&& self, F&&) -> Expected {
        return util::forward<Self>(self);
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F>
    requires(concepts::ConstructibleFrom<Expected, Self>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::fmap_right>, Self&& self, F&&) -> Expected {
        return util::forward<Self>(self);
    }
};
}
