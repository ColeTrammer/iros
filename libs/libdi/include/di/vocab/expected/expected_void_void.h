#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/language_void.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/function/monad/monad_interface.h>
#include <di/vocab/expected/expected_forward_declaration.h>

namespace di::vocab {
template<>
class Expected<void, void> : public function::monad::MonadInterface<Expected<void, void>> {
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

    constexpr void error() const& {}
    constexpr void error() && {}

    constexpr void emplace() {}

private:
    template<typename G>
    constexpr friend bool operator==(Expected const& a, Expected<void, G> const& b) {
        return b.has_value();
    }

    template<typename G>
    constexpr friend bool operator==(Expected const&, Unexpected<G> const&) {
        return false;
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F, typename U = meta::UnwrapRefDecay<meta::InvokeResult<F>>>
    constexpr friend Expected<U, void> tag_invoke(types::Tag<function::monad::fmap>, Self&& self, F&& function) {
        if constexpr (concepts::LanguageVoid<U>) {
            function::invoke(util::forward<F>(function));
            return {};
        } else {
            return function::invoke(util::forward<F>(function));
        }
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F, typename R = meta::InvokeResult<F>>
    requires(concepts::Expected<R>)
    constexpr friend R tag_invoke(types::Tag<function::monad::bind>, Self&& self, F&& function) {
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