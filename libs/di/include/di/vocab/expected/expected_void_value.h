#pragma once

#include "di/assert/assert_bool.h"
#include "di/function/invoke.h"
#include "di/function/monad/monad_interface.h"
#include "di/meta/compare.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/meta/vocab.h"
#include "di/util/forward.h"
#include "di/util/initializer_list.h"
#include "di/util/move.h"
#include "di/util/swap.h"
#include "di/vocab/expected/expected_can_convert_constructor.h"
#include "di/vocab/expected/expected_forward_declaration.h"
#include "di/vocab/expected/expected_void_void.h"
#include "di/vocab/expected/unexpect.h"
#include "di/vocab/expected/unexpected.h"
#include "di/vocab/optional/optional.h"

namespace di::vocab {
template<typename E>
class [[nodiscard]] Expected<void, E> : public function::monad::MonadInterface<Expected<void, E>> {
private:
    using ErrorStorage = Optional<E>;

public:
    using Value = void;
    using Error = E;
    using UnexpectedType = Unexpected<E>;

    constexpr Expected() = default;

    constexpr Expected(Expected const&)
    requires(concepts::CopyConstructible<ErrorStorage>)
    = default;
    constexpr Expected(Expected const&)
    requires(!concepts::CopyConstructible<ErrorStorage>)
    = delete;

    constexpr Expected(Expected&&)
    requires(concepts::MoveConstructible<ErrorStorage>)
    = default;

    template<typename F>
    requires(concepts::ConstructibleFrom<E, F const&> &&
             concepts::detail::ExpectedCanConvertConstructor<void, E, void, F>)
    constexpr explicit(!concepts::ConvertibleTo<F const&, E>) Expected(Expected<void, F> const& other)
        : m_error(other.m_error) {}

    template<typename F>
    requires(concepts::ConstructibleFrom<E, F> && concepts::detail::ExpectedCanConvertConstructor<void, E, void, F>)
    constexpr explicit(!concepts::ConvertibleTo<F, E>) Expected(Expected<void, F>&& other)
        : m_error(util::move(other).m_error) {}

    template<typename G>
    requires(concepts::ConstructibleFrom<E, G const&>)
    constexpr explicit(!concepts::ConvertibleTo<G const&, E>) Expected(Unexpected<G> const& error)
        : m_error(error.error()) {}

    template<typename G>
    requires(concepts::ConstructibleFrom<E, G>)
    constexpr explicit(!concepts::ConvertibleTo<G, E>) Expected(Unexpected<G>&& error)
        : m_error(util::move(error).error()) {}

    constexpr explicit Expected(types::InPlace) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<E, Args...>)
    constexpr explicit Expected(types::Unexpect, Args&&... args)
        : m_error(types::in_place, util::forward<Args>(args)...) {}

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<E, std::initializer_list<U>, Args...>)
    constexpr explicit Expected(types::Unexpect, std::initializer_list<U> list, Args&&... args)
        : m_error(types::in_place, list, util::forward<Args>(args)...) {}

    constexpr ~Expected() = default;

    constexpr auto operator=(Expected const&) -> Expected& requires(!concepts::CopyConstructible<E>) = delete;
    constexpr auto operator=(Expected const&) -> Expected& requires(concepts::CopyConstructible<E>) = default;

    constexpr auto operator=(Expected&&) -> Expected& requires(concepts::MoveConstructible<E>) = default;

    template<typename G>
    requires(concepts::ConstructibleFrom<E, G const&>)
    constexpr auto operator=(Unexpected<G> const& error) -> Expected& {
        m_error = error.error();
        return *this;
    }

    template<typename G>
    requires(concepts::ConstructibleFrom<E, G>)
    constexpr auto operator=(Unexpected<G>&& error) -> Expected& {
        m_error = util::move(error).error();
        return *this;
    }

    constexpr explicit operator bool() const { return has_value(); }
    constexpr auto has_value() const -> bool { return !m_error.has_value(); }

    constexpr void emplace() { m_error.reset(); }

    constexpr void operator*() const { DI_ASSERT(has_value()); }

    constexpr void value() const& { DI_ASSERT(has_value()); }

    constexpr void value() && { DI_ASSERT(has_value()); }

    constexpr auto error() & -> E& { return *m_error; }
    constexpr auto error() const& -> E const& { return *m_error; }
    constexpr auto error() && -> E&& { return *util::move(m_error); }
    constexpr auto error() const&& -> E const&& { return *util::move(m_error); }

    constexpr auto __try_did_fail() && -> Unexpected<E> {
        return Unexpected<E> { in_place, util::move(*this).error() };
    }
    constexpr auto __try_did_succeed() && -> Expected { return Expected {}; }
    constexpr void __try_move_out() && { return util::move(*this).value(); }

private:
    template<typename U, typename G>
    friend class Expected;

    constexpr friend void tag_invoke(types::Tag<util::swap>, Expected& a, Expected& b)
    requires(concepts::Swappable<E>)
    {
        util::swap(a.m_value, b.m_value);
    }

    template<concepts::EqualityComparableWith<E> G>
    constexpr friend auto operator==(Expected const& a, Expected<void, G> const& b) -> bool {
        if (a.has_value() != b.has_value()) {
            return false;
        }
        return a.has_value() || a.error() == b.error();
    }

    template<concepts::EqualityComparableWith<E> G>
    constexpr friend auto operator==(Expected const& a, Unexpected<G> const& b) -> bool {
        return !a && a.error() == b.error();
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename U = meta::UnwrapRefDecay<meta::InvokeResult<F>>>
    requires(concepts::ConstructibleFrom<E, meta::Like<Self, E>>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::fmap>, Self&& self, F&& function) -> Expected<U, E> {
        if (!self) {
            return Expected<U, E> { types::unexpect, util::forward<Self>(self).error() };
        }
        if constexpr (concepts::LanguageVoid<U>) {
            function::invoke(util::forward<F>(function));
            return {};
        } else {
            return function::invoke(util::forward<F>(function));
        }
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F, typename R = meta::InvokeResult<F>>
    requires(concepts::Expected<R> && concepts::ConvertibleTo<meta::Like<Self, E>, meta::ExpectedError<R>>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::bind>, Self&& self, F&& function) -> R {
        if (!self) {
            return R { types::unexpect, util::forward<Self>(self).error() };
        }
        return function::invoke(util::forward<F>(function));
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename R = meta::InvokeResult<F, meta::Like<Self, E>>>
    requires(concepts::Expected<R> && concepts::LanguageVoid<meta::ExpectedValue<R>>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::fail>, Self&& self, F&& function) -> R {
        if (self) {
            return {};
        }
        return function::invoke(util::forward<F>(function), util::forward<Self>(self).error());
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename G = meta::UnwrapRefDecay<meta::InvokeResult<F, meta::Like<Self, E>>>>
    constexpr friend auto tag_invoke(types::Tag<function::monad::fmap_right>, Self&& self, F&& function)
        -> Expected<void, G> {
        if (self) {
            return {};
        }
        if constexpr (concepts::LanguageVoid<G>) {
            function::invoke(util::forward<F>(function), util::forward<Self>(self).error());
            return {};
        } else {
            return Expected<void, G> { types::unexpect, function::invoke(util::forward<F>(function),
                                                                         util::forward<Self>(self).error()) };
        }
    }

    Optional<E> m_error;
};
}
