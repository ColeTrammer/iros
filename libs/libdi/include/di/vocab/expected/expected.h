#pragma once

#include <di/assert/prelude.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/default_constructible.h>
#include <di/concepts/expected.h>
#include <di/concepts/language_void.h>
#include <di/concepts/move_constructible.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/concepts/trivially_copy_constructible.h>
#include <di/concepts/trivially_destructible.h>
#include <di/concepts/trivially_move_constructible.h>
#include <di/concepts/unexpected.h>
#include <di/function/monad/monad_interface.h>
#include <di/meta/expected_error.h>
#include <di/meta/expected_rank.h>
#include <di/meta/like.h>
#include <di/meta/unwrap_ref_decay.h>
#include <di/util/address_of.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/forward.h>
#include <di/util/initializer_list.h>
#include <di/util/move.h>
#include <di/util/rebindable_box.h>
#include <di/vocab/expected/expected_can_convert_constructor.h>
#include <di/vocab/expected/expected_forward_declaration.h>
#include <di/vocab/expected/unexpect.h>
#include <di/vocab/expected/unexpected.h>
#include <di/vocab/optional/prelude.h>

namespace di::vocab {
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
    requires(concepts::ConstructibleFrom<T, U const&> && concepts::ConstructibleFrom<E, G const&> &&
             concepts::detail::ExpectedCanConvertConstructor<T, E, U, G>)
    constexpr explicit(!concepts::ConvertibleTo<U const&, T> || !concepts::ConvertibleTo<G const&, E>)
        Expected(Expected<U, G> const& other) {
        internal_construct_from_expected(other);
    }

    template<typename U, typename G>
    requires(concepts::ConstructibleFrom<T, U> && concepts::ConstructibleFrom<E, G> &&
             concepts::detail::ExpectedCanConvertConstructor<T, E, U, G>)
    constexpr explicit(!concepts::ConvertibleTo<U, T> || !concepts::ConvertibleTo<G, E>)
        Expected(Expected<U, G>&& other) {
        internal_construct_from_expected(util::move(other));
    }

    template<typename U = T>
    requires(!concepts::RemoveCVRefSameAs<U, types::InPlace> && !concepts::RemoveCVRefSameAs<U, Expected> &&
             !concepts::Unexpected<U> && concepts::ConstructibleFrom<T, U>)
    constexpr explicit(!concepts::ConvertibleTo<U, T>) Expected(U&& value)
        : m_has_error(false), m_value(util::forward<U>(value)) {}

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
        : m_has_error(false), m_value(types::in_place, util::forward<Args>(args)...) {}

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<T, util::InitializerList<U>, Args...>)
    constexpr explicit Expected(types::InPlace, util::InitializerList<U> list, Args&&... args)
        : m_has_error(false), m_value(types::in_place, list, util::forward<Args>(args)...) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<E, Args...>)
    constexpr explicit Expected(types::Unexpect, Args&&... args)
        : m_has_error(true), m_error(types::in_place, util::forward<Args>(args)...) {}

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<E, util::InitializerList<U>, Args...>)
    constexpr explicit Expected(types::Unexpect, util::InitializerList<U> list, Args&&... args)
        : m_has_error(true), m_error(types::in_place, list, util::forward<Args>(args)...) {}

    constexpr ~Expected() = default;

    constexpr ~Expected()
    requires(!concepts::TriviallyDestructible<T> || !concepts::TriviallyDestructible<E>)
    {
        internal_reset();
    }

    constexpr Expected& operator=(Expected const& other)
    requires(concepts::CopyConstructible<T> && concepts::CopyConstructible<E>)
    {
        return internal_assign_from_expected(other);
    }

    constexpr Expected& operator=(Expected&& other)
    requires(concepts::MoveConstructible<T> && concepts::MoveConstructible<E>)
    {
        return internal_assign_from_expected(util::move(other));
    }

    template<typename U = T>
    requires(!concepts::RemoveCVRefSameAs<Expected, U> && !concepts::Unexpected<U> && concepts::ConstructibleFrom<T, U>)
    constexpr Expected& operator=(U&& value) {
        return internal_assign_from_value(util::forward<U>(value));
    }

    template<typename G>
    requires(concepts::ConstructibleFrom<E, G const&>)
    constexpr Expected& operator=(Unexpected<G> const& error) {
        return internal_assign_from_unexpected(error);
    }

    template<typename G>
    requires(concepts::ConstructibleFrom<E, G>)
    constexpr Expected& operator=(Unexpected<G>&& error) {
        return internal_assign_from_unexpected(util::move(error));
    }

    constexpr auto operator->() { return util::address_of(value()); }
    constexpr auto operator->() const { return util::address_of(value()); }

    constexpr T& operator*() & { return value(); }
    constexpr T const& operator*() const& { return value(); }
    constexpr T&& operator*() && { return util::move(*this).value(); }
    constexpr T const&& operator*() const&& { return util::move(*this).value(); }

    constexpr explicit operator bool() const { return has_value(); }
    constexpr bool has_value() const { return !m_has_error; }

    constexpr T& value() & {
        DI_ASSERT(has_value());
        return m_value.value();
    }
    constexpr T const& value() const& {
        DI_ASSERT(has_value());
        return m_value.value();
    }
    constexpr T&& value() && {
        DI_ASSERT(has_value());
        return util::move(m_value).value();
    }
    constexpr T const&& value() const&& {
        DI_ASSERT(has_value());
        return util::move(m_value).value();
    }

    constexpr E& error() & {
        DI_ASSERT(!has_value());
        return m_error.value();
    }
    constexpr E const& error() const& {
        DI_ASSERT(!has_value());
        return m_error.value();
    }
    constexpr E&& error() && {
        DI_ASSERT(!has_value());
        return util::move(m_error).value();
    }
    constexpr E const&& error() const&& {
        DI_ASSERT(!has_value());
        return util::move(m_error).value();
    }

    template<concepts::ConvertibleTo<T> U>
    requires(concepts::CopyConstructible<T>)
    constexpr T value_or(U&& default_value) const& {
        return has_value() ? **this : static_cast<T>(util::forward<U>(default_value));
    }

    template<concepts::ConvertibleTo<T> U>
    requires(concepts::MoveConstructible<T>)
    constexpr T value_or(U&& default_value) && {
        return has_value() ? *util::move(*this) : static_cast<T>(util::forward<U>(default_value));
    }

    constexpr Optional<T> optional_value() const&
    requires(concepts::CopyConstructible<T>)
    {
        return has_value() ? Optional<T> { in_place, **this } : nullopt;
    }

    constexpr Optional<T> optional_value() &&
        requires(concepts::MoveConstructible<T>)
    {
        return has_value() ? Optional<T> { in_place, *util::move(*this) } : nullopt;
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr T& emplace(Args&&... args) {
        return internal_emplace(util::forward<Args>(args)...);
    }

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<T, util::InitializerList<U>, Args...>)
    constexpr T& emplace(util::InitializerList<U> list, Args&&... args) {
        return internal_emplace(list, util::forward<Args>(args)...);
    }

    constexpr Unexpected<E> __try_did_fail() && { return Unexpected<E> { in_place, util::move(*this).error() }; }
    constexpr Expected __try_did_succeed() && { return Expected { in_place, util::move(*this).value() }; }
    constexpr T&& __try_move_out() && { return util::move(*this).value(); }

private:
    template<typename U, typename G>
    friend class Expected;

    template<concepts::EqualityComparableWith<T> U, concepts::EqualityComparableWith<E> G>
    constexpr friend bool operator==(Expected const& a, Expected<U, G> const& b) {
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
    constexpr friend bool operator==(Expected const& a, U const& b) {
        return a.has_value() && a.value() == b;
    }

    template<concepts::EqualityComparableWith<E> G>
    constexpr friend bool operator==(Expected const& a, Unexpected<G> const& b) {
        return !a.has_value() && a.error() == b.error();
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename U = meta::UnwrapRefDecay<meta::InvokeResult<F, meta::Like<Self, T>>>>
    requires(concepts::ConstructibleFrom<E, meta::Like<Self, E>>)
    constexpr friend Expected<U, E> tag_invoke(types::Tag<function::monad::fmap>, Self&& self, F&& function) {
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
    constexpr friend R tag_invoke(types::Tag<function::monad::bind>, Self&& self, F&& function) {
        if (!self) {
            return R { types::unexpect, util::forward<Self>(self).error() };
        }
        return function::invoke(util::forward<F>(function), util::forward<Self>(self).value());
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename R = meta::InvokeResult<F, meta::Like<Self, E>>>
    requires(concepts::Expected<R> && concepts::ConvertibleTo<meta::Like<Self, T>, meta::ExpectedValue<R>>)
    constexpr friend R tag_invoke(types::Tag<function::monad::fail>, Self&& self, F&& function) {
        if (self) {
            return R { types::in_place, util::forward<Self>(self).value() };
        }
        return function::invoke(util::forward<F>(function), util::forward<Self>(self).error());
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename G = meta::UnwrapRefDecay<meta::InvokeResult<F, meta::Like<Self, E>>>>
    constexpr friend Expected<T, G> tag_invoke(types::Tag<function::monad::fmap_right>, Self&& self, F&& function) {
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
        if (other.has_value()) {
            util::construct_at(&this->m_value, util::forward<U>(other).value());
        } else {
            util::construct_at(&this->m_error, util::forward<U>(other).error());
        }
        this->m_has_error = !other.has_value();
    }

    constexpr void internal_reset() {
        if (has_value()) {
            util::destroy_at(&m_value);
        } else {
            util::destroy_at(&m_error);
        }
    }

    template<typename U>
    constexpr Expected& internal_assign_from_expected(U&& other) {
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
    constexpr Expected& internal_assign_from_value(U&& value) {
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
    constexpr Expected& internal_assign_from_unexpected(U&& unexpected) {
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
    constexpr T& internal_emplace(Args&&... args) {
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
    constexpr T& internal_emplace(util::InitializerList<U> list, Args&&... args) {
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
