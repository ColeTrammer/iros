#pragma once

#include <di/function/monad/monad_interface.h>
#include <di/meta/compare.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/meta/vocab.h>
#include <di/util/addressof.h>
#include <di/util/forward.h>
#include <di/util/move.h>
#include <di/util/rebindable_box.h>
#include <di/util/unreachable.h>
#include <di/vocab/expected/expected_can_convert_constructor.h>
#include <di/vocab/expected/expected_void_void.h>
#include <di/vocab/optional/prelude.h>

namespace di::vocab {
// Expected<T, void> is used to denote an expected value
// which is known at compile to not contain an error, and
// is thus not equivalent to Optional<T>. This is especially
// for code which can be generic over the fallibility of certain
// operations, while still accounting for potential errors, but
// not wrapping its return value in expected unless necessary.
template<typename T>
class [[nodiscard]] Expected<T, void> : public function::monad::MonadInterface<Expected<T, void>> {
public:
    using Value = T;
    using Error = void;

    constexpr Expected()
    requires(concepts::DefaultConstructible<T>)
    = default;

    constexpr Expected(Expected const&)
    requires(!concepts::CopyConstructible<T>)
    = delete;
    constexpr Expected(Expected const&) = default;

    constexpr Expected(Expected&&)
    requires(concepts::MoveConstructible<T>)
    = default;

    template<typename U>
    requires(concepts::ConstructibleFrom<T, U const&> &&
             concepts::detail::ExpectedCanConvertConstructor<T, void, U, void>)
    constexpr explicit(!concepts::ConvertibleTo<U const&, T>) Expected(Expected<U, void> const& other)
        : m_value(other.value()) {}

    template<typename U>
    requires(concepts::ConstructibleFrom<T, U> && concepts::detail::ExpectedCanConvertConstructor<T, void, U, void>)
    constexpr explicit(!concepts::ConvertibleTo<U, T>) Expected(Expected<U, void>&& other)
        : m_value(util::move(other).value()) {}

    template<typename U = T>
    requires(!concepts::RemoveCVRefSameAs<U, types::InPlace> && !concepts::RemoveCVRefSameAs<U, Expected> &&
             !concepts::Unexpected<U> && concepts::ConstructibleFrom<T, U>)
    constexpr explicit(!concepts::ConvertibleTo<U, T>) Expected(U&& value) : m_value(util::forward<U>(value)) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit Expected(types::InPlace, Args&&... args) : m_value(util::forward<Args>(args)...) {}

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr explicit Expected(types::InPlace, std::initializer_list<U> list, Args&&... args)
        : m_value(list, util::forward<Args>(args)...) {}

    constexpr ~Expected() = default;

    constexpr Expected& operator=(Expected const&)
    requires(!concepts::CopyConstructible<T>)
    = delete;
    constexpr Expected& operator=(Expected const&)
    requires(concepts::CopyConstructible<T>)
    = default;

    constexpr Expected& operator=(Expected&&)
    requires(concepts::MoveConstructible<T>)
    = default;

    template<typename U = T>
    requires(!concepts::RemoveCVRefSameAs<Expected, U> && !concepts::Unexpected<U> && concepts::ConstructibleFrom<T, U>)
    constexpr Expected& operator=(U&& value) {
        m_value = util::forward<U>(value);
        return *this;
    }

    constexpr auto operator->() { return util::addressof(m_value.value()); }
    constexpr auto operator->() const { return util::addressof(m_value.value()); }

    constexpr T& operator*() & { return m_value.value(); }
    constexpr T const& operator*() const& { return m_value.value(); }
    constexpr T&& operator*() && { return util::move(m_value).value(); }
    constexpr T const&& operator*() const&& { return util::move(m_value).value(); }

    constexpr explicit operator bool() const { return true; }
    constexpr bool has_value() const { return true; }

    constexpr T& value() & { return m_value.value(); }
    constexpr T const& value() const& { return m_value.value(); }
    constexpr T&& value() && { return util::move(m_value).value(); }
    constexpr T const&& value() const&& { return util::move(m_value).value(); }

    constexpr void error() const& {}
    constexpr void error() && {}

    template<concepts::ConvertibleTo<T> U>
    requires(concepts::CopyConstructible<T>)
    constexpr T value_or(U&&) const& {
        return **this;
    }

    template<concepts::ConvertibleTo<T> U>
    requires(concepts::MoveConstructible<T>)
    constexpr T value_or(U&&) && {
        return *util::move(*this);
    }

    constexpr auto optional_value() const { return Optional<void>(has_value()); }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr T& emplace(Args&&... args) {
        return m_value.emplace(util::forward<Args>(args)...);
    }

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr T& emplace(std::initializer_list<U> list, Args&&... args) {
        return m_value.emplace(list, util::forward<Args>(args)...);
    }

    Expected __try_did_fail() && { util::unreachable(); }
    constexpr Expected __try_did_succeed() && { return Expected { in_place, util::move(*this).value() }; }
    constexpr T&& __try_move_out() && { return util::move(*this).value(); }

private:
    template<typename U, typename G>
    friend class Expected;

    constexpr friend void tag_invoke(types::Tag<util::swap>, Expected& a, Expected& b)
    requires(concepts::Swappable<T>)
    {
        util::swap(a.m_value, b.m_value);
    }

    template<concepts::EqualityComparableWith<T> U, typename G>
    constexpr friend bool operator==(Expected const& a, Expected<U, G> const& b) {
        return b.has_value() && *a == *b;
    }

    template<typename U>
    requires(meta::ExpectedRank<U> < meta::ExpectedRank<Expected> && concepts::EqualityComparableWith<T, U>)
    constexpr friend bool operator==(Expected const& a, U const& b) {
        return a.value() == b;
    }

    template<typename G>
    constexpr friend bool operator==(Expected const&, Unexpected<G> const&) {
        return false;
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename U = meta::UnwrapRefDecay<meta::InvokeResult<F, meta::Like<Self, T>>>>
    constexpr friend Expected<U, void> tag_invoke(types::Tag<function::monad::fmap>, Self&& self, F&& function) {
        if constexpr (concepts::LanguageVoid<U>) {
            function::invoke(util::forward<F>(function), util::forward<Self>(self).value());
            return {};
        } else {
            return Expected<U, void>(in_place,
                                     function::invoke(util::forward<F>(function), util::forward<Self>(self).value()));
        }
    }

    template<concepts::RemoveCVRefSameAs<Expected> Self, typename F,
             typename R = meta::InvokeResult<F, meta::Like<Self, T>>>
    requires(concepts::Expected<R>)
    constexpr friend R tag_invoke(types::Tag<function::monad::bind>, Self&& self, F&& function) {
        return function::invoke(util::forward<F>(function), util::forward<Self>(self).value());
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

    util::RebindableBox<T> m_value {};
};

template<typename T>
Expected(T&&) -> Expected<meta::UnwrapRefDecay<T>, void>;
}
