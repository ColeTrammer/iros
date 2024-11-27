#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/meta/enable_view.h>
#include <di/function/invoke.h>
#include <di/function/monad/monad_interface.h>
#include <di/meta/compare.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/meta/trivial.h>
#include <di/meta/util.h>
#include <di/meta/vocab.h>
#include <di/types/in_place.h>
#include <di/util/addressof.h>
#include <di/util/declval.h>
#include <di/util/initializer_list.h>
#include <di/util/move.h>
#include <di/util/swap.h>
#include <di/vocab/optional/constructible_from_cref_optional.h>
#include <di/vocab/optional/optional_forward_declaration.h>
#include <di/vocab/optional/storage_for.h>

namespace di::vocab {
namespace detail {
    template<typename Opt, typename From, typename To>
    concept OptionalConvertibleToWorkaround =
        !concepts::SameAs<Opt, meta::RemoveCVRef<From>> && concepts::ConvertibleTo<From, To>;
}

template<typename T>
requires(!concepts::LanguageVoid<T>)
class Optional<T>
    : public meta::EnableView<Optional<T>>
    , public meta::EnableBorrowedContainer<Optional<T>, concepts::LValueReference<T>>
    , public function::monad::MonadInterface<Optional<T>> {
private:
    static_assert(!concepts::RValueReference<T>);

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
    constexpr auto operator=(Optional const&) -> Optional& = default;
    constexpr auto operator=(Optional&&) -> Optional& = default;
    constexpr ~Optional() = default;

    constexpr Optional(Optional const& other)
    requires(concepts::Copyable<T> && !concepts::TriviallyCopyConstructible<Storage>)
    {
        if (other.has_value()) {
            emplace(other.value());
        }
    }

    constexpr Optional(Optional&& other)
    requires(!concepts::TriviallyMoveConstructible<Storage>)
    {
        if (other.has_value()) {
            emplace(util::move(other).value());
        }
    }

    template<typename U>
    requires(!concepts::SameAs<T, U> && concepts::ConstructibleFrom<T, U const&> &&
             !ConstructibleFromCRefOptional<T, Optional<U>>)
    constexpr explicit(!concepts::ImplicitlyConvertibleTo<U const&, T>) Optional(Optional<U> const& other) {
        if (other.has_value()) {
            emplace(other.value());
        }
    }

    template<typename U>
    requires(!concepts::SameAs<T, U> && concepts::ConstructibleFrom<T, U &&> &&
             !ConstructibleFromCRefOptional<T, Optional<U>>)
    constexpr explicit(!detail::OptionalConvertibleToWorkaround<Optional, U&&, T>) Optional(Optional<U>&& other) {
        if (other.has_value()) {
            emplace(util::move(other).value());
        }
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr Optional(types::InPlace, Args&&... args) {
        emplace(util::forward<Args>(args)...);
    }

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr Optional(types::InPlace, std::initializer_list<U> list, Args&&... args) {
        emplace(list, util::forward<Args>(args)...);
    }

    template<typename U = T>
    requires(!concepts::OneOf<meta::Decay<U>, Optional, types::InPlace> && concepts::ConstructibleFrom<T, U &&>)
    constexpr explicit(!detail::OptionalConvertibleToWorkaround<Optional, U&&, T>) Optional(U&& value) {
        emplace(util::forward<U>(value));
    }

    constexpr ~Optional()
    requires(!concepts::TriviallyDestructible<Storage>)
    {
        reset();
    }

    constexpr auto operator=(NullOpt) -> Optional& {
        reset();
        return *this;
    }

    constexpr auto operator=(Optional const& other)
        -> Optional& requires(concepts::Copyable<T> && !concepts::TriviallyCopyAssignable<Storage>) {
            if (other.has_value()) {
                emplace(other.value());
            }
            return *this;
        }

    constexpr auto operator=(Optional&& other) -> Optional& requires(!concepts::TriviallyMoveAssignable<Storage>) {
        if (other.has_value()) {
            emplace(util::move(other).value());
        }
        return *this;
    }

    template<typename U = T>
    requires(concepts::ConstructibleFrom<T, U> && !concepts::SameAs<meta::RemoveCVRef<U>, Optional> &&
             (!concepts::Scalar<T> || !concepts::SameAs<meta::Decay<U>, T>) )
    constexpr auto operator=(U&& value) -> Optional& {
        emplace(util::forward<U>(value));
        return *this;
    }

    template<typename U>
    requires(concepts::ConstructibleFrom<T, U const&> && !ConstructibleFromCRefOptional<T, Optional<U>>)
    constexpr auto operator=(Optional<U> const& other) -> Optional& {
        if (other.has_value()) {
            emplace(other.value());
        }
        return *this;
    }

    template<typename U>
    requires(concepts::ConstructibleFrom<T, U &&> && !ConstructibleFromCRefOptional<T, Optional<U>>)
    constexpr auto operator=(Optional<U>&& other) -> Optional& {
        if (other.has_value()) {
            emplace(util::move(other).value());
        }
        return *this;
    }

    // Accessors
    constexpr auto has_value() const -> bool { return !is_nullopt(m_storage); }
    constexpr explicit operator bool() const { return has_value(); }

    using Reference = decltype(get_value(util::declval<Storage&>()));
    using ConstReference = decltype(get_value(util::declval<Storage const&>()));

    using Pointer = decltype(util::addressof(util::declval<Reference>()));
    using ConstPointer = decltype(util::addressof(util::declval<ConstReference>()));

    constexpr auto operator->() -> Pointer { return util::addressof(value()); }
    constexpr auto operator->() const -> ConstPointer { return util::addressof(value()); }

    constexpr auto operator*() & -> decltype(auto) { return value(); }
    constexpr auto operator*() const& -> decltype(auto) { return value(); }
    constexpr auto operator*() && -> decltype(auto) { return util::move(*this).value(); }
    constexpr auto operator*() const&& -> decltype(auto) { return util::move(*this).value(); }

    constexpr auto value() & -> decltype(auto) {
        DI_ASSERT(has_value());
        return get_value(m_storage);
    }
    constexpr auto value() const& -> decltype(auto) {
        DI_ASSERT(has_value());
        return get_value(m_storage);
    }
    constexpr auto value() && -> decltype(auto) {
        DI_ASSERT(has_value());
        return get_value(util::move(m_storage));
    }
    constexpr auto value() const&& -> decltype(auto) {
        DI_ASSERT(has_value());
        return get_value(util::move(m_storage));
    }

    template<concepts::ConvertibleTo<T> U>
    requires(concepts::Copyable<T>)
    constexpr auto value_or(U&& fallback) const& -> T {
        return has_value() ? value() : static_cast<T>(util::forward<U>(fallback));
    }

    template<concepts::ConvertibleTo<T> U>
    constexpr auto value_or(U&& fallback) && -> T {
        return has_value() ? util::move(*this).value() : static_cast<T>(util::forward<U>(fallback));
    }

    // Container interface
    constexpr auto begin() -> Pointer {
        if (!has_value()) {
            return nullptr;
        }
        return util::addressof(value());
    }

    constexpr auto begin() const -> ConstPointer {
        if (!has_value()) {
            return nullptr;
        }
        return util::addressof(value());
    }

    constexpr auto end() -> Pointer {
        if (!has_value()) {
            return nullptr;
        }
        return util::addressof(value()) + 1;
    }

    constexpr auto end() const -> ConstPointer {
        if (!has_value()) {
            return nullptr;
        }
        return util::addressof(value()) + 1;
    }

    constexpr auto empty() const -> bool { return !has_value(); }
    constexpr auto size() const -> types::size_t { return has_value(); }

    constexpr auto data() -> Pointer { return begin(); }
    constexpr auto data() const -> ConstPointer { return begin(); }

    constexpr auto front() -> Optional<Reference> {
        if (!has_value()) {
            return nullopt;
        }
        return **this;
    }
    constexpr auto front() const -> Optional<ConstReference> {
        if (!has_value()) {
            return nullopt;
        }
        return **this;
    }

    constexpr auto back() -> Optional<Reference> { return front(); }
    constexpr auto back() const -> Optional<ConstReference> { return front(); }

    constexpr auto operator[](types::ssize_t index) -> Reference { return *at(index); }
    constexpr auto operator[](types::ssize_t index) const -> ConstReference { return *at(index); }

    constexpr auto at(types::ssize_t index) -> Optional<Reference> {
        if (index != 0) {
            return nullopt;
        }
        return front();
    }

    constexpr auto at(types::ssize_t index) const -> Optional<ConstReference> {
        if (index != 0) {
            return nullopt;
        }
        return front();
    }

    constexpr void reset() { set_nullopt(m_storage); }

    template<typename... Args>
    constexpr auto emplace(Args&&... args) -> decltype(auto) {
        reset();
        set_value(m_storage, util::forward<Args>(args)...);
        return **this;
    }

private:
    constexpr friend void tag_invoke(types::Tag<util::swap>, Optional& a, Optional& b)
    requires(concepts::Swappable<T>)
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

    // Monadic interface
    template<concepts::DecaySameAs<Optional> Self, concepts::Invocable<meta::Like<Self, Value>> F,
             typename R = meta::InvokeResult<F, meta::Like<Self, Value>>>
    requires(concepts::Optional<R>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::bind>, Self&& self, F&& f) -> R {
        if (self.has_value()) {
            return function::invoke(util::forward<F>(f), util::forward<Self>(self).value());
        } else {
            return R();
        }
    }

    template<concepts::DecaySameAs<Optional> Self, concepts::Invocable<meta::Like<Self, Value>> F,
             typename R = meta::UnwrapRefDecay<meta::InvokeResult<F, meta::Like<Self, Value>>>>
    constexpr friend auto tag_invoke(types::Tag<function::monad::fmap>, Self&& self, F&& f) -> Optional<R> {
        if (self.has_value()) {
            if constexpr (concepts::LanguageVoid<R>) {
                function::invoke(util::forward<F>(f), util::forward<Self>(self).value());
                return Optional<R>(types::in_place);
            } else {
                return Optional<R>(types::in_place,
                                   function::invoke(util::forward<F>(f), util::forward<Self>(self).value()));
            }
        } else {
            return nullopt;
        }
    }

    template<concepts::DecaySameAs<Optional> Self, concepts::InvocableTo<Optional> F>
    requires(concepts::ConstructibleFrom<Optional, Self>)
    constexpr friend auto tag_invoke(types::Tag<function::monad::fail>, Self&& self, F&& f) -> Optional {
        return self.has_value() ? util::forward<Self>(self) : function::invoke(util::forward<F>(f));
    }

    Storage m_storage { nullopt };
};

template<typename T, concepts::EqualityComparableWith<T> U>
constexpr auto operator==(Optional<T> const& a, Optional<U> const& b) -> bool {
    return (!a && !b) || (a && b && *a == *b);
}

template<typename T>
constexpr auto operator==(Optional<T> const& a, NullOpt) -> bool {
    return !a;
}

template<typename T, typename U>
requires((meta::OptionalRank<T> >= meta::OptionalRank<U>) && concepts::EqualityComparableWith<T, U>)
constexpr auto operator==(Optional<T> const& a, U const& b) -> bool {
    return a.has_value() && *a == b;
}

template<typename T, concepts::ThreeWayComparableWith<T> U>
constexpr auto operator<=>(Optional<T> const& a, Optional<U> const& b) -> meta::CompareThreeWayResult<T, U> {
    if (!a && !b) {
        return types::strong_ordering::equal;
    }
    if (auto result = a.has_value() <=> b.has_value(); result != 0) {
        return result;
    }
    return *a <=> *b;
}

template<typename T>
constexpr auto operator<=>(Optional<T> const& a, NullOpt) -> types::strong_ordering {
    return a.has_value() <=> false;
}

template<typename T, typename U>
requires((meta::OptionalRank<T> >= meta::OptionalRank<U>) && concepts::ThreeWayComparableWith<T, U>)
constexpr auto operator<=>(Optional<T> const& a, U const& b) -> meta::CompareThreeWayResult<T, U> {
    if (!a.has_value()) {
        return types::strong_ordering::less;
    }
    return *a <=> b;
}

template<class T>
Optional(T) -> Optional<T>;
}
