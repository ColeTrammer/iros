#pragma once

#include <di/util/addressof.h>
#include <di/util/concepts/copyable.h>
#include <di/util/concepts/explicitly_convertible.h>
#include <di/util/concepts/one_of.h>
#include <di/util/concepts/scalar.h>
#include <di/util/concepts/trivially_copy_assignable.h>
#include <di/util/concepts/trivially_copy_constructible.h>
#include <di/util/concepts/trivially_destructible.h>
#include <di/util/concepts/trivially_move_assignable.h>
#include <di/util/concepts/trivially_move_constructible.h>
#include <di/util/invoke.h>
#include <di/util/meta/decay.h>
#include <di/util/meta/remove_cvref.h>
#include <di/util/move.h>
#include <di/util/swap.h>
#include <di/util/types.h>
#include <di/vocab/optional/constructible_from_cref_optional.h>
#include <di/vocab/optional/storage_for.h>

namespace di::vocab::optional {
template<typename T>
class Optional {
private:
    using Storage = StorageFor<T>;
    static_assert(OptionalStorage<Storage, T>);

public:
    using Value = T;

    constexpr Optional() = default;
    constexpr Optional(NullOpt) {}

    // Conditionally trivial special member functions. These overloads will
    // be selected when the special member functions defined below are deleted.
    constexpr Optional(Optional const&) = default;
    constexpr Optional(Optional&&) = default;
    constexpr Optional& operator=(Optional const&) = default;
    constexpr Optional& operator=(Optional&&) = default;
    constexpr ~Optional() = default;

    constexpr Optional(Optional const& other)
    requires(util::concepts::Copyable<T> && !util::concepts::TriviallyCopyConstructible<Storage>)
    {
        if (other.has_value()) {
            emplace(other.value());
        }
    }

    constexpr Optional(Optional&& other)
    requires(!util::concepts::TriviallyMoveConstructible<Storage>)
    {
        if (other.has_value()) {
            emplace(util::move(other).value());
        }
    }

    template<typename U>
    requires(util::concepts::ConstructibleFrom<T, U const&> && !ConstructibleFromCRefOptional<T, Optional<U>>)
    constexpr explicit(!util::concepts::ImplicitlyConvertibleTo<U const&, T>) Optional(Optional<U> const& other) {
        if (other.has_value()) {
            emplace(other.value());
        }
    }

    template<typename U>
    requires(util::concepts::ConstructibleFrom<T, U &&> && !ConstructibleFromCRefOptional<T, Optional<U>>)
    constexpr explicit(!util::concepts::ImplicitlyConvertibleTo<U&&, T>) Optional(Optional<U>&& other) {
        if (other.has_value()) {
            emplace(util::move(other).value());
        }
    }

    template<typename... Args>
    constexpr Optional(util::InPlace, Args&&... args) {
        emplace(util::forward<Args>(args)...);
    }

    template<typename U = T>
    requires(util::concepts::ConstructibleFrom<T, U &&> && !util::concepts::OneOf<util::meta::Decay<U>, Optional, util::InPlace>)
    constexpr explicit(!util::concepts::ImplicitlyConvertibleTo<U&&, T>) Optional(U&& value) {
        emplace(util::forward<U>(value));
    }

    constexpr ~Optional()
    requires(!util::concepts::TriviallyDestructible<Storage>)
    {
        reset();
    }

    constexpr Optional& operator=(NullOpt) {
        reset();
        return *this;
    }

    constexpr Optional& operator=(Optional const& other)
    requires(util::concepts::Copyable<T> && !util::concepts::TriviallyCopyAssignable<Storage>)
    {
        if (other.has_value()) {
            emplace(other.value());
        }
        return *this;
    }

    constexpr Optional& operator=(Optional&& other)
    requires(!util::concepts::TriviallyMoveAssignable<Storage>)
    {
        if (other.has_value()) {
            emplace(util::move(other).value());
        }
        return *this;
    }

    template<typename U = T>
    requires(util::concepts::ConstructibleFrom<T, U> && !util::concepts::SameAs<util::meta::RemoveCVRef<U>, Optional> &&
             (!util::concepts::Scalar<T> || !util::concepts::SameAs<util::meta::Decay<U>, T>) )
    constexpr Optional& operator=(U&& value) {
        emplace(util::forward<U>(value));
        return *this;
    }

    template<typename U>
    requires(util::concepts::ConstructibleFrom<T, U const&> && !ConstructibleFromCRefOptional<T, Optional<U>>)
    constexpr Optional& operator=(Optional<U> const& other) {
        if (other.has_value()) {
            emplace(other.value());
        }
        return *this;
    }

    template<typename U>
    requires(util::concepts::ConstructibleFrom<T, U &&> && !ConstructibleFromCRefOptional<T, Optional<U>>)
    constexpr Optional& operator=(Optional<U>&& other) {
        if (other.has_value()) {
            emplace(util::move(other).value());
        }
        return *this;
    }

    constexpr bool has_value() const { return !is_nullopt(m_storage); }
    constexpr explicit operator bool() const { return has_value(); }

    constexpr auto operator->() { return util::addressof(value()); }
    constexpr auto operator->() const { return util::addressof(value()); }

    constexpr decltype(auto) operator*() & { return value(); }
    constexpr decltype(auto) operator*() const& { return value(); }
    constexpr decltype(auto) operator*() && { return util::move(*this).value(); }
    constexpr decltype(auto) operator*() const&& { return util::move(*this).value(); }

    constexpr decltype(auto) value() & { return get_value(m_storage); }
    constexpr decltype(auto) value() const& { return get_value(m_storage); }
    constexpr decltype(auto) value() && { return get_value(util::move(m_storage)); }
    constexpr decltype(auto) value() const&& { return get_value(util::move(m_storage)); }

    template<util::concepts::ExplicitlyConvertibleTo<T> U>
    requires(util::concepts::Copyable<T>)
    constexpr T value_or(U&& fallback) const& {
        return has_value() ? value() : static_cast<T>(util::forward<U>(fallback));
    }

    template<util::concepts::ExplicitlyConvertibleTo<T> U>
    constexpr T value_or(U&& fallback) && {
        return has_value() ? util::move(*this).value() : static_cast<T>(util::forward<U>(fallback));
    }

    template<util::concepts::Invocable<OptionalGetValue<Storage&>> F, typename R = util::meta::InvokeResult<F, OptionalGetValue<Storage&>>>
    requires(util::concepts::Optional<R>)
    constexpr R and_then(F&& f) & {
        if (has_value()) {
            return util::invoke(util::forward<F>(f), value());
        } else {
            return R();
        }
    }

    template<util::concepts::Invocable<OptionalGetValue<Storage const&>> F,
             typename R = util::meta::InvokeResult<F, OptionalGetValue<Storage const&>>>
    requires(util::concepts::Optional<R>)
    constexpr R and_then(F&& f) const& {
        if (has_value()) {
            return util::invoke(util::forward<F>(f), value());
        } else {
            return R();
        }
    }

    template<util::concepts::Invocable<OptionalGetValue<Storage&&>> F,
             typename R = util::meta::InvokeResult<F, OptionalGetValue<Storage&&>>>
    requires(util::concepts::Optional<R>)
    constexpr R and_then(F&& f) && {
        if (has_value()) {
            return util::invoke(util::forward<F>(f), util::move(*this).value());
        } else {
            return R();
        }
    }

    template<util::concepts::Invocable<OptionalGetValue<Storage const&&>> F,
             typename R = util::meta::InvokeResult<F, OptionalGetValue<Storage const&&>>>
    requires(util::concepts::Optional<R>)
    constexpr R and_then(F&& f) const&& {
        if (has_value()) {
            return util::invoke(util::forward<F>(f), util::move(*this).value());
        } else {
            return R();
        }
    }

    template<util::concepts::Invocable<OptionalGetValue<Storage&>> F, typename R = util::meta::InvokeResult<F, OptionalGetValue<Storage&>>>
    constexpr Optional<R> transform(F&& f) & {
        if (has_value()) {
            return Optional<R>(util::in_place, util::invoke(util::forward<F>(f), value()));
        } else {
            return nullopt;
        }
    }

    template<util::concepts::Invocable<OptionalGetValue<Storage const&>> F,
             typename R = util::meta::InvokeResult<F, OptionalGetValue<Storage const&>>>
    constexpr Optional<R> transform(F&& f) const& {
        if (has_value()) {
            return Optional<R>(util::in_place, util::invoke(util::forward<F>(f), value()));
        } else {
            return nullopt;
        }
    }

    template<util::concepts::Invocable<OptionalGetValue<Storage&&>> F,
             typename R = util::meta::InvokeResult<F, OptionalGetValue<Storage&&>>>
    constexpr Optional<R> transform(F&& f) && {
        if (has_value()) {
            return Optional<R>(util::in_place, util::invoke(util::forward<F>(f), util::move(*this).value()));
        } else {
            return nullopt;
        }
    }

    template<util::concepts::Invocable<OptionalGetValue<Storage const&&>> F,
             typename R = util::meta::InvokeResult<F, OptionalGetValue<Storage const&&>>>
    constexpr Optional<R> transform(F&& f) const&& {
        if (has_value()) {
            return Optional<R>(util::in_place, util::invoke(util::forward<F>(f), util::move(*this).value()));
        } else {
            return nullopt;
        }
    }

    template<util::concepts::InvocableTo<Optional> F>
    requires(util::concepts::Copyable<Optional>)
    constexpr Optional or_else(F&& f) const& {
        return *this ? *this : util::invoke(util::forward<F>(f));
    }

    template<util::concepts::InvocableTo<Optional> F>
    requires(util::concepts::Movable<Optional>)
    constexpr Optional or_else(F&& f) && {
        return *this ? util::move(*this) : util::invoke(util::forward<F>(f));
    }

    constexpr void reset() { set_nullopt(m_storage); }

    template<typename... Args>
    constexpr decltype(auto) emplace(Args&&... args) {
        reset();
        set_value(m_storage, util::forward<Args>(args)...);
        return **this;
    }

private:
    constexpr friend void tag_invoke(util::Tag<util::swap>, Optional& a, Optional& b)
    requires(util::concepts::Swappable<T>)
    {
        if (a.has_value() && b.has_value()) {
            util::swap(a.m_storage, b.m_storage);
        } else if (a.has_value()) {
            b = util::move(a);
            a.reset();
        } else if (b.has_value()) {
            a = util::move(b);
            b.reset();
        }
    }

    Storage m_storage { nullopt };
};

template<class T>
Optional(T) -> Optional<T>;
}
