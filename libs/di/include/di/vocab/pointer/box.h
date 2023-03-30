#pragma once

#include <di/assert/prelude.h>
#include <di/concepts/language_array.h>
#include <di/platform/prelude.h>
#include <di/types/prelude.h>
#include <di/util/exchange.h>
#include <di/util/std_new.h>
#include <di/vocab/error/result.h>

namespace di::vocab {
template<typename T>
struct DefaultDelete {
    DefaultDelete() = default;

    template<typename U>
    requires(concepts::ConvertibleTo<U*, T*>)
    constexpr DefaultDelete(DefaultDelete<U> const&) {}

    constexpr void operator()(T* pointer) const { delete pointer; }
};

template<typename T, typename Deleter = DefaultDelete<T>>
class Box {
public:
    Box()
    requires(concepts::DefaultConstructible<Deleter> && !concepts::Pointer<Deleter>)
    = default;

    constexpr Box(nullptr_t)
    requires(concepts::DefaultConstructible<Deleter> && !concepts::Pointer<Deleter>)
    {}

    constexpr explicit Box(T* pointer)
    requires(concepts::DefaultConstructible<Deleter> && !concepts::Pointer<Deleter>)
        : m_pointer(pointer) {}

    Box(Box const&) = delete;
    constexpr Box(Box&& other)
    requires(concepts::MoveConstructible<Deleter>)
        : m_pointer(other.release()), m_deleter(util::forward<Deleter>(other.get_deleter())) {}

    constexpr Box(T* pointer, Deleter const& deleter)
    requires(concepts::CopyConstructible<Deleter>)
        : m_pointer(pointer), m_deleter(deleter) {}

    template<concepts::MoveConstructible D = Deleter>
    requires(!concepts::LValueReference<Deleter>)
    constexpr Box(T* pointer, D&& deleter) : m_pointer(pointer, util::forward<D>(deleter)) {}

    template<typename D = Deleter>
    requires(concepts::LValueReference<Deleter>)
    Box(T*, meta::RemoveReference<D>&&) = delete;

    template<typename U, typename E>
    requires(concepts::ImplicitlyConvertibleTo<U*, T*> && !concepts::LanguageArray<U> &&
             ((concepts::Reference<Deleter> && concepts::SameAs<E, Deleter>) ||
              (!concepts::Reference<Deleter> && concepts::ImplicitlyConvertibleTo<E, Deleter>) ))
    constexpr Box(Box<U, E>&& other) : m_pointer(other.release()), m_deleter(util::forward<E>(other.get_deleter())) {}

    constexpr ~Box() { reset(); }

    Box& operator=(Box const&) = delete;
    constexpr Box& operator=(Box&& other)
    requires(concepts::MoveAssignable<Deleter>)
    {
        reset(other.release());
        m_deleter = util::forward<Deleter>(other.get_deleter());
        return *this;
    }

    template<typename U, typename E>
    requires(concepts::ImplicitlyConvertibleTo<U*, T*> && !concepts::LanguageArray<U> &&
             concepts::AssignableFrom<Deleter&, E &&>)
    constexpr Box& operator=(Box<U, E>&& other) {
        reset(other.release());
        m_deleter = util::forward<Deleter>(other.get_deleter());
        return *this;
    }

    constexpr Box& operator=(nullptr_t) {
        reset();
        return *this;
    }

    constexpr T* release() { return util::exchange(m_pointer, nullptr); }
    constexpr void reset(T* pointer = nullptr) {
        auto* old_pointer = m_pointer;
        m_pointer = pointer;
        if (old_pointer) {
            function::invoke(m_deleter, old_pointer);
        }
    }

    constexpr T* get() const { return m_pointer; }

    constexpr Deleter& get_deleter() { return m_deleter; }
    constexpr Deleter const& get_deleter() const { return m_deleter; }

    constexpr explicit operator bool() const { return !!m_pointer; }

    constexpr T& operator*() const {
        DI_ASSERT(*this);
        return *get();
    }
    constexpr T* operator->() const {
        DI_ASSERT(*this);
        return get();
    }

private:
    template<typename U>
    constexpr friend bool operator==(Box const& a, Box<U> const& b) {
        return const_cast<void*>(static_cast<void const volatile*>(a.get())) ==
               const_cast<void*>(static_cast<void const volatile*>(b.get()));
    }

    template<typename U>
    constexpr friend auto operator<=>(Box const& a, Box<U> const& b) {
        return const_cast<void*>(static_cast<void const volatile*>(a.get())) <=>
               const_cast<void*>(static_cast<void const volatile*>(b.get()));
    }

    constexpr friend bool operator==(Box const& a, nullptr_t) { return a.get() == static_cast<T*>(nullptr); }
    constexpr friend auto operator<=>(Box const& a, nullptr_t) {
        return a == nullptr ? di::strong_ordering::equal : di::strong_ordering::greater;
    }

    T* m_pointer { nullptr };
    [[no_unique_address]] Deleter m_deleter {};
};

template<typename T, typename... Args>
requires(!concepts::LanguageArray<T> && concepts::ConstructibleFrom<T, Args...>)
constexpr DefaultFallibleNewResult<Box<T>> try_box(Args&&... args) {
    if consteval {
        return Box<T>(new T(util::forward<Args>(args)...));
    }
    auto* result = new (std::nothrow) T(util::forward<Args>(args)...);
    if (!result) {
        return vocab::Unexpected(platform::default_fallible_allocation_error());
    }
    return Box<T>(result);
}

template<typename T, typename... Args>
requires(!concepts::LanguageArray<T> && concepts::ConstructibleFrom<T, Args...>)
constexpr auto make_box(Args&&... args) {
    return *try_box<T>(util::forward<Args>(args)...);
}
}
