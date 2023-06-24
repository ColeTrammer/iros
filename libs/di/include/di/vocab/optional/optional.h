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
    constexpr Optional& operator=(Optional const&) = default;
    constexpr Optional& operator=(Optional&&) = default;
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

    constexpr Optional& operator=(NullOpt) {
        reset();
        return *this;
    }

    constexpr Optional& operator=(Optional const& other)
    requires(concepts::Copyable<T> && !concepts::TriviallyCopyAssignable<Storage>)
    {
        if (other.has_value()) {
            emplace(other.value());
        }
        return *this;
    }

    constexpr Optional& operator=(Optional&& other)
    requires(!concepts::TriviallyMoveAssignable<Storage>)
    {
        if (other.has_value()) {
            emplace(util::move(other).value());
        }
        return *this;
    }

    template<typename U = T>
    requires(concepts::ConstructibleFrom<T, U> && !concepts::SameAs<meta::RemoveCVRef<U>, Optional> &&
             (!concepts::Scalar<T> || !concepts::SameAs<meta::Decay<U>, T>) )
    constexpr Optional& operator=(U&& value) {
        emplace(util::forward<U>(value));
        return *this;
    }

    template<typename U>
    requires(concepts::ConstructibleFrom<T, U const&> && !ConstructibleFromCRefOptional<T, Optional<U>>)
    constexpr Optional& operator=(Optional<U> const& other) {
        if (other.has_value()) {
            emplace(other.value());
        }
        return *this;
    }

    template<typename U>
    requires(concepts::ConstructibleFrom<T, U &&> && !ConstructibleFromCRefOptional<T, Optional<U>>)
    constexpr Optional& operator=(Optional<U>&& other) {
        if (other.has_value()) {
            emplace(util::move(other).value());
        }
        return *this;
    }

    // Accessors
    constexpr bool has_value() const { return !is_nullopt(m_storage); }
    constexpr explicit operator bool() const { return has_value(); }

    using Reference = decltype(get_value(util::declval<Storage&>()));
    using ConstReference = decltype(get_value(util::declval<Storage const&>()));

    using Pointer = decltype(util::addressof(util::declval<Reference>()));
    using ConstPointer = decltype(util::addressof(util::declval<ConstReference>()));

    constexpr Pointer operator->() { return util::addressof(value()); }
    constexpr ConstPointer operator->() const { return util::addressof(value()); }

    constexpr decltype(auto) operator*() & { return value(); }
    constexpr decltype(auto) operator*() const& { return value(); }
    constexpr decltype(auto) operator*() && { return util::move(*this).value(); }
    constexpr decltype(auto) operator*() const&& { return util::move(*this).value(); }

    constexpr decltype(auto) value() & {
        DI_ASSERT(has_value());
        return get_value(m_storage);
    }
    constexpr decltype(auto) value() const& {
        DI_ASSERT(has_value());
        return get_value(m_storage);
    }
    constexpr decltype(auto) value() && {
        DI_ASSERT(has_value());
        return get_value(util::move(m_storage));
    }
    constexpr decltype(auto) value() const&& {
        DI_ASSERT(has_value());
        return get_value(util::move(m_storage));
    }

    template<concepts::ConvertibleTo<T> U>
    requires(concepts::Copyable<T>)
    constexpr T value_or(U&& fallback) const& {
        return has_value() ? value() : static_cast<T>(util::forward<U>(fallback));
    }

    template<concepts::ConvertibleTo<T> U>
    constexpr T value_or(U&& fallback) && {
        return has_value() ? util::move(*this).value() : static_cast<T>(util::forward<U>(fallback));
    }

    // Container interface
    constexpr Pointer begin() {
        if (!has_value()) {
            return nullptr;
        }
        return util::addressof(value());
    }

    constexpr ConstPointer begin() const {
        if (!has_value()) {
            return nullptr;
        }
        return util::addressof(value());
    }

    constexpr Pointer end() {
        if (!has_value()) {
            return nullptr;
        }
        return util::addressof(value()) + 1;
    }

    constexpr ConstPointer end() const {
        if (!has_value()) {
            return nullptr;
        }
        return util::addressof(value()) + 1;
    }

    constexpr bool empty() const { return !has_value(); }
    constexpr types::size_t size() const { return has_value(); }

    constexpr Pointer data() { return begin(); }
    constexpr ConstPointer data() const { return begin(); }

    constexpr Optional<Reference> front() {
        if (!has_value()) {
            return nullopt;
        }
        return **this;
    }
    constexpr Optional<ConstReference> front() const {
        if (!has_value()) {
            return nullopt;
        }
        return **this;
    }

    constexpr Optional<Reference> back() { return front(); }
    constexpr Optional<ConstReference> back() const { return front(); }

    constexpr Reference operator[](types::ssize_t index) { return *at(index); }
    constexpr ConstReference operator[](types::ssize_t index) const { return *at(index); }

    constexpr Optional<Reference> at(types::ssize_t index) {
        if (index != 0) {
            return nullopt;
        }
        return front();
    }

    constexpr Optional<ConstReference> at(types::ssize_t index) const {
        if (index != 0) {
            return nullopt;
        }
        return front();
    }

    constexpr void reset() { set_nullopt(m_storage); }

    template<typename... Args>
    constexpr decltype(auto) emplace(Args&&... args) {
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
    template<concepts::DecaySameAs<Optional> Self, concepts::Invocable<OptionalGetValue<meta::Like<Self, Storage>>> F,
             typename R = meta::InvokeResult<F, OptionalGetValue<meta::Like<Self, Storage>>>>
    requires(concepts::Optional<R>)
    constexpr friend R tag_invoke(types::Tag<function::monad::bind>, Self&& self, F&& f) {
        if (self.has_value()) {
            return function::invoke(util::forward<F>(f), util::forward<Self>(self).value());
        } else {
            return R();
        }
    }

    template<concepts::DecaySameAs<Optional> Self, concepts::Invocable<OptionalGetValue<meta::Like<Self, Storage>>> F,
             typename R = meta::UnwrapRefDecay<meta::InvokeResult<F, OptionalGetValue<meta::Like<Self, Storage>>>>>
    constexpr friend Optional<R> tag_invoke(types::Tag<function::monad::fmap>, Self&& self, F&& f) {
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
    constexpr friend Optional tag_invoke(types::Tag<function::monad::fail>, Self&& self, F&& f) {
        return self.has_value() ? util::forward<Self>(self) : function::invoke(util::forward<F>(f));
    }

    Storage m_storage { nullopt };
};

template<typename T, concepts::EqualityComparableWith<T> U>
constexpr bool operator==(Optional<T> const& a, Optional<U> const& b) {
    return (!a && !b) || (a && b && *a == *b);
}

template<typename T>
constexpr bool operator==(Optional<T> const& a, NullOpt) {
    return !a;
}

template<typename T, typename U>
requires((meta::OptionalRank<T> >= meta::OptionalRank<U>) && concepts::EqualityComparableWith<T, U>)
constexpr bool operator==(Optional<T> const& a, U const& b) {
    return a.has_value() && *a == b;
}

template<typename T, concepts::ThreeWayComparableWith<T> U>
constexpr meta::CompareThreeWayResult<T, U> operator<=>(Optional<T> const& a, Optional<U> const& b) {
    if (!a && !b) {
        return types::strong_ordering::equal;
    }
    if (auto result = a.has_value() <=> b.has_value(); result != 0) {
        return result;
    }
    return *a <=> *b;
}

template<typename T>
constexpr types::strong_ordering operator<=>(Optional<T> const& a, NullOpt) {
    return a.has_value() <=> false;
}

template<typename T, typename U>
requires((meta::OptionalRank<T> >= meta::OptionalRank<U>) && concepts::ThreeWayComparableWith<T, U>)
constexpr meta::CompareThreeWayResult<T, U> operator<=>(Optional<T> const& a, U const& b) {
    if (!a.has_value()) {
        return types::strong_ordering::less;
    }
    return *a <=> b;
}

template<class T>
Optional(T) -> Optional<T>;
}
