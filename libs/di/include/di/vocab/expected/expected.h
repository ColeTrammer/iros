#pragma once

#include "di/assert/assert_bool.h"
#include "di/function/monad/monad_interface.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/meta/operations.h"
#include "di/meta/trivial.h"
#include "di/meta/util.h"
#include "di/meta/vocab.h"
#include "di/util/addressof.h"
#include "di/util/construct_at.h"
#include "di/util/destroy_at.h"
#include "di/util/forward.h"
#include "di/util/initializer_list.h"
#include "di/util/move.h"
#include "di/util/rebindable_box.h"
#include "di/vocab/expected/expected_can_convert_constructor.h"
#include "di/vocab/expected/expected_forward_declaration.h"
#include "di/vocab/expected/unexpect.h"
#include "di/vocab/expected/unexpected.h"
#include "di/vocab/optional/prelude.h"

namespace di::vocab {
namespace detail {
    template<typename Expected, typename From, typename To>
    concept ConvertibleToWorkaround =
        !concepts::SameAs<Expected, meta::RemoveCVRef<From>> && concepts::ConvertibleTo<From, To>;
}

template<typename T, typename E>
requires(!concepts::LanguageVoid<T> && !concepts::LanguageVoid<E>)
class [[nodiscard]] Expected<T, E> : public function::monad::MonadInterface<Expected<T, E>> {
public:
    using Value = T;
    using Error = E;

    constexpr Expected()
    requires(concepts::DefaultConstructible<T>)
        : m_value() {}

    constexpr Expected(Expected const&) = default;
    constexpr Expected(Expected&&) = default;

    constexpr Expected(Expected const&)
    requires(!concepts::CopyConstructible<T> || !concepts::CopyConstructible<E>)
    = delete;

    constexpr Expected(Expected const& other)
    requires((!concepts::TriviallyCopyConstructible<T> || !concepts::TriviallyCopyConstructible<E>) &&
             concepts::CopyConstructible<T> && concepts::CopyConstructible<E>)
    {
        internal_construct_from_expected(other);
    }

    constexpr Expected(Expected&& other)
    requires((!concepts::TriviallyMoveConstructible<T> || !concepts::TriviallyMoveConstructible<E>) &&
             concepts::MoveConstructible<T> && concepts::MoveConstructible<E>)
    {
        internal_construct_from_expected(util::move(other));
    }

    template<typename U, typename G>
    requires((!concepts::SameAs<U, T> || !concepts::SameAs<G, E>) && concepts::ConstructibleFrom<T, U const&> &&
             concepts::ConstructibleFrom<E, G const&> && concepts::detail::ExpectedCanConvertConstructor<T, E, U, G>)
    constexpr explicit(!concepts::ConvertibleTo<U const&, T> || !concepts::ConvertibleTo<G const&, E>)
        Expected(Expected<U, G> const& other) {
        internal_construct_from_expected(other);
    }

    template<typename U, typename G>
    requires((!concepts::SameAs<U, T> || !concepts::SameAs<G, E>) && concepts::ConstructibleFrom<T, U> &&
             concepts::ConstructibleFrom<E, G> && concepts::detail::ExpectedCanConvertConstructor<T, E, U, G>)
    constexpr explicit(!concepts::ConvertibleTo<U, T> || !concepts::ConvertibleTo<G, E>)
        Expected(Expected<U, G>&& other) {
        internal_construct_from_expected(util::move(other));
    }

    template<typename U = T>
    requires(!concepts::RemoveCVRefSameAs<U, types::InPlace> && !concepts::RemoveCVRefSameAs<U, Expected> &&
             !concepts::Unexpected<U> && concepts::ConstructibleFrom<T, U>)
    constexpr explicit(!detail::ConvertibleToWorkaround<Expected, U, T>) Expected(U&& value)
        : m_value(util::forward<U>(value)) {}

    template<typename G>
    requires(concepts::ConstructibleFrom<E, G const&>)
    constexpr explicit(!concepts::ConvertibleTo<G const&, E>) Expected(Unexpected<G> const& error)
        : m_has_error(true), m_error(error.error()) {}

    template<typename G>
    requires(concepts::ConstructibleFrom<E, G>)
    constexpr explicit(!concepts::ConvertibleTo<G, E>) Expected(Unexpected<G>&& error)
        : m_has_error(true), m_error(util::move(error).error()) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit Expected(types::InPlace, Args&&... args)
        : m_value(types::in_place, util::forward<Args>(args)...) {}

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr explicit Expected(types::InPlace, std::initializer_list<U> list, Args&&... args)
        : m_value(types::in_place, list, util::forward<Args>(args)...) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<E, Args...>)
    constexpr explicit Expected(types::Unexpect, Args&&... args)
        : m_has_error(true), m_error(types::in_place, util::forward<Args>(args)...) {}

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<E, std::initializer_list<U>, Args...>)
    constexpr explicit Expected(types::Unexpect, std::initializer_list<U> list, Args&&... args)
        : m_has_error(true), m_error(types::in_place, list, util::forward<Args>(args)...) {}

    constexpr ~Expected() = default;

    constexpr ~Expected()
    requires(!concepts::TriviallyDestructible<T> || !concepts::TriviallyDestructible<E>)
    {
        internal_reset();
    }

    constexpr auto operator=(Expected const& other)
        -> Expected& requires(concepts::CopyConstructible<T>&& concepts::CopyConstructible<E>) {
            return internal_assign_from_expected(other);
        }

    constexpr auto operator=(Expected&& other)
        -> Expected& requires(concepts::MoveConstructible<T>&& concepts::MoveConstructible<E>) {
            return internal_assign_from_expected(util::move(other));
        }

    template<typename U = T>
    requires(!concepts::RemoveCVRefSameAs<Expected, U> && !concepts::Unexpected<U> && concepts::ConstructibleFrom<T, U>)
    constexpr auto operator=(U&& value) -> Expected& {
        return internal_assign_from_value(util::forward<U>(value));
    }

    template<typename G>
    requires(concepts::ConstructibleFrom<E, G const&>)
    constexpr auto operator=(Unexpected<G> const& error) -> Expected& {
        return internal_assign_from_unexpected(error);
    }

    template<typename G>
    requires(concepts::ConstructibleFrom<E, G>)
    constexpr auto operator=(Unexpected<G>&& error) -> Expected& {
        return internal_assign_from_unexpected(util::move(error));
    }

    constexpr auto operator->() { return util::addressof(value()); }
    constexpr auto operator->() const { return util::addressof(value()); }

    constexpr auto operator*() & -> T& { return value(); }
    constexpr auto operator*() const& -> T const& { return value(); }
    constexpr auto operator*() && -> T&& { return util::move(*this).value(); }
    constexpr auto operator*() const&& -> T const&& { return util::move(*this).value(); }

    constexpr explicit operator bool() const { return has_value(); }
    constexpr auto has_value() const -> bool { return !m_has_error; }

    constexpr auto value() & -> T& {
        DI_ASSERT(has_value());
        return m_value.value();
    }
    constexpr auto value() const& -> T const& {
        DI_ASSERT(has_value());
        return m_value.value();
    }
    constexpr auto value() && -> T&& {
        DI_ASSERT(has_value());
        return util::move(m_value).value();
    }
    constexpr auto value() const&& -> T const&& {
        DI_ASSERT(has_value());
        return util::move(m_value).value();
    }

    constexpr auto error() & -> E& {
        DI_ASSERT(!has_value());
        return m_error.value();
    }
    constexpr auto error() const& -> E const& {
        DI_ASSERT(!has_value());
        return m_error.value();
    }
    constexpr auto error() && -> E&& {
        DI_ASSERT(!has_value());
        return util::move(m_error).value();
    }
    constexpr auto error() const&& -> E const&& {
        DI_ASSERT(!has_value());
        return util::move(m_error).value();
    }

    template<concepts::ConvertibleTo<T> U>
    requires(concepts::CopyConstructible<T>)
    constexpr auto value_or(U&& default_value) const& -> T {
        return has_value() ? **this : static_cast<T>(util::forward<U>(default_value));
    }

    template<concepts::ConvertibleTo<T> U>
    requires(concepts::MoveConstructible<T>)
    constexpr auto value_or(U&& default_value) && -> T {
        return has_value() ? *util::move(*this) : static_cast<T>(util::forward<U>(default_value));
    }

    constexpr auto optional_value() const& -> Optional<T>
    requires(concepts::CopyConstructible<T>)
    {
        return has_value() ? Optional<T> { in_place, **this } : nullopt;
    }

    constexpr auto optional_value() && -> Optional<T>
    requires(concepts::MoveConstructible<T>)
    {
        return has_value() ? Optional<T> { in_place, *util::move(*this) } : nullopt;
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr auto emplace(Args&&... args) -> T& {
        return internal_emplace(util::forward<Args>(args)...);
    }

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr auto emplace(std::initializer_list<U> list, Args&&... args) -> T& {
        return internal_emplace(list, util::forward<Args>(args)...);
    }

    constexpr auto __try_did_fail() && -> Unexpected<E> {
        return Unexpected<E> { in_place, util::move(*this).error() };
    }
    constexpr auto __try_did_succeed() && -> Expected { return Expected { in_place, util::move(*this).value() }; }
    constexpr auto __try_move_out() && -> T&& { return util::move(*this).value(); }

private:
    template<typename U, typename G>
    friend class Expected;

    template<concepts::EqualityComparableWith<T> U, concepts::EqualityComparableWith<E> G>
    constexpr friend auto operator==(Expected const& a, Expected<U, G> const& b) -> bool {
        if (a.has_value() != b.has_value()) {
            return false;
        }
        if (a.has_value() == b.has_value()) {
            return a.value() == b.value();
        }
        return a.error() == b.error();
    }

    template<typename U>
    requires(meta::ExpectedRank<U> < meta::ExpectedRank<Expected> && concepts::EqualityComparableWith<T, U>)
    constexpr friend auto operator==(Expected const& a, U const& b) -> bool {
        return a.has_value() && a.value() == b;
    }

    template<concepts::EqualityComparableWith<E> G>
    constexpr friend auto operator==(Expected const& a, Unexpected<G> const& b) -> bool {
        return !a.has_value() && a.error() == b.error();
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename U = meta::UnwrapRefDecay<meta::InvokeResult<F, meta::Like<Self, T>>>>
    requires(concepts::ConstructibleFrom<E, meta::Like<Self, E>>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::fmap>, Self&& self, F&& function) -> Expected<U, E> {
        if (!self) {
            return Expected<U, E> { types::unexpect, util::forward<Self>(self).error() };
        }
        if constexpr (concepts::LanguageVoid<U>) {
            function::invoke(util::forward<F>(function), util::forward<Self>(self).value());
            return {};
        } else {
            return function::invoke(util::forward<F>(function), util::forward<Self>(self).value());
        }
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename R = meta::InvokeResult<F, meta::Like<Self, T>>>
    requires(concepts::Expected<R> && concepts::ConvertibleTo<meta::Like<Self, E>, meta::ExpectedError<R>>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::bind>, Self&& self, F&& function) -> R {
        if (!self) {
            return R { types::unexpect, util::forward<Self>(self).error() };
        }
        return function::invoke(util::forward<F>(function), util::forward<Self>(self).value());
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename R = meta::InvokeResult<F, meta::Like<Self, E>>>
    requires(concepts::Expected<R> && concepts::ConvertibleTo<meta::Like<Self, T>, meta::ExpectedValue<R>>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::fail>, Self&& self, F&& function) -> R {
        if (self) {
            return R { types::in_place, util::forward<Self>(self).value() };
        }
        return function::invoke(util::forward<F>(function), util::forward<Self>(self).error());
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename G = meta::UnwrapRefDecay<meta::InvokeResult<F, meta::Like<Self, E>>>>
    constexpr friend auto tag_invoke(types::Tag<function::monad::fmap_right>, Self&& self, F&& function)
        -> Expected<T, G> {
        if (self) {
            return Expected<T, G> { types::in_place, util::forward<Self>(self).value() };
        }
        if constexpr (concepts::LanguageVoid<G>) {
            function::invoke(util::forward<F>(function), util::forward<Self>(self).error());
            return Expected<T, G> { types::in_place, util::forward<Self>(self).value() };
        } else {
            return Expected<T, G> { types::unexpect,
                                    function::invoke(util::forward<F>(function), util::forward<Self>(self).error()) };
        }
    }

    template<typename U>
    constexpr void internal_construct_from_expected(U&& other) {
        auto other_has_value = other.has_value();
        if (other_has_value) {
            util::construct_at(&this->m_value, di::forward<U>(other).value());
        } else {
            util::construct_at(&this->m_error, di::forward<U>(other).error());
        }
        this->m_has_error = !other_has_value;
    }

    constexpr void internal_reset() {
        if (has_value()) {
            util::destroy_at(&m_value);
        } else {
            util::destroy_at(&m_error);
        }
    }

    template<typename U>
    constexpr auto internal_assign_from_expected(U&& other) -> Expected& {
        if (this->has_value() && other.has_value()) {
            this->m_value = util::forward<U>(other).value();
        } else if (this->has_value() && !other.has_value()) {
            internal_reset();
            util::construct_at(&this->m_error, util::forward<U>(other).error());
        } else if (!this->has_value() && other.has_value()) {
            internal_reset();
            util::construct_at(&this->m_value, util::forward<U>(other).value());
        } else {
            this->m_error = util::forward<U>(other).error();
        }
        this->m_has_error = !other.has_value();
        return *this;
    }

    template<typename U>
    constexpr auto internal_assign_from_value(U&& value) -> Expected& {
        if (this->has_value()) {
            this->m_value = util::forward<U>(value);
        } else {
            internal_reset();
            util::construct_at(&this->m_value, util::forward<U>(value));
        }
        this->m_has_error = false;
        return *this;
    }

    template<typename U>
    constexpr auto internal_assign_from_unexpected(U&& unexpected) -> Expected& {
        if (this->has_value()) {
            internal_reset();
            util::construct_at(&this->m_error, util::forward<U>(unexpected).error());
        } else {
            this->m_error = util::forward<U>(unexpected).error();
        }
        this->m_has_error = true;
        return *this;
    }

    template<typename... Args>
    constexpr auto internal_emplace(Args&&... args) -> T& {
        if (this->has_value()) {
            m_value.emplace(util::forward<Args>(args)...);
        } else {
            internal_reset();
            util::construct_at(&this->m_value, types::in_place, util::forward<Args>(args)...);
        }
        this->m_has_error = false;
        return value();
    }

    template<typename U, typename... Args>
    constexpr auto internal_emplace(std::initializer_list<U> list, Args&&... args) -> T& {
        if (this->has_value()) {
            m_value.emplace(list, util::forward<Args>(args)...);
        } else {
            internal_reset();
            util::construct_at(&this->m_value, types::in_place, list, util::forward<Args>(args)...);
        }
        this->m_has_error = false;
        return value();
    }

    bool m_has_error { false };
    union {
        util::RebindableBox<T> m_value;
        util::RebindableBox<E> m_error;
    };
};
}
