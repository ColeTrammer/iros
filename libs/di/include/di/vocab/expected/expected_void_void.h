#pragma once

#include <di/function/monad/monad_interface.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/in_place.h>
#include <di/util/unreachable.h>
#include <di/vocab/expected/expected_forward_declaration.h>

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

    constexpr Expected& operator=(Expected const&) = default;
    constexpr Expected& operator=(Expected&&) = default;

    constexpr explicit operator bool() const { return true; }
    constexpr bool has_value() const { return true; }

    constexpr void operator*() const& {}
    constexpr void operator*() && {}

    constexpr void value() const& {}
    constexpr void value() && {}

    constexpr void emplace() {}

    Expected __try_did_fail() && { util::unreachable(); }
    constexpr Expected __try_did_succeed() && { return Expected {}; }
    constexpr void __try_move_out() && {}

private:
    template<typename G>
    constexpr friend bool operator==(Expected const&, Expected<void, G> const& b) {
        return b.has_value();
    }

    template<typename G>
    constexpr friend bool operator==(Expected const&, Unexpected<G> const&) {
        return false;
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename U = meta::UnwrapRefDecay<meta::InvokeResult<F>>>
    constexpr friend Expected<U, void> tag_invoke(types::Tag<function::monad::fmap>, Self&&, F&& function) {
        if constexpr (concepts::LanguageVoid<U>) {
            function::invoke(util::forward<F>(function));
            return {};
        } else {
            return function::invoke(util::forward<F>(function));
        }
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F, typename R = meta::InvokeResult<F>>
    requires(concepts::Expected<R>)
    constexpr friend R tag_invoke(types::Tag<function::monad::bind>, Self&&, F&& function) {
        return function::invoke(util::forward<F>(function));
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F>
    requires(concepts::ConstructibleFrom<Expected, Self>)
    constexpr friend Expected tag_invoke(types::Tag<function::monad::fail>, Self&& self, F&&) {
        return util::forward<Self>(self);
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F>
    requires(concepts::ConstructibleFrom<Expected, Self>)
    constexpr friend Expected tag_invoke(types::Tag<function::monad::fmap_right>, Self&& self, F&&) {
        return util::forward<Self>(self);
    }
};
}
