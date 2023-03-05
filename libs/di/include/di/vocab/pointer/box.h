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
requires(!concepts::LanguageArray<T>)
class Box {
public:
    Box() = default;

    constexpr Box(nullptr_t) {}
    constexpr explicit Box(T* pointer) : m_pointer(pointer) {}

    Box(Box const&) = delete;
    constexpr Box(Box&& other) : m_pointer(other.release()) {}

    template<typename U>
    requires(concepts::ImplicitlyConvertibleTo<U*, T*>)
    constexpr Box(Box<U>&& other) : m_pointer(other.release()) {}

    constexpr ~Box() { reset(); }

    Box& operator=(Box const&) = delete;
    constexpr Box& operator=(Box&& other) {
        reset(other.release());
        return *this;
    }

    template<typename U>
    requires(concepts::ImplicitlyConvertibleTo<U*, T*>)
    constexpr Box& operator=(Box<U>&& other) {
        reset(other.release());
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
            platform::DefaultFallibleAllocator<T>().deallocate(old_pointer, 1);
        }
    }

    constexpr T* get() const { return m_pointer; }

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
        return const_cast<void*>(static_cast<const volatile void*>(a.get())) ==
               const_cast<void*>(static_cast<const volatile void*>(b.get()));
    }

    template<typename U>
    constexpr friend auto operator<=>(Box const& a, Box<U> const& b) {
        return const_cast<void*>(static_cast<const volatile void*>(a.get())) <=>
               const_cast<void*>(static_cast<const volatile void*>(b.get()));
    }

    constexpr friend bool operator==(Box const& a, nullptr_t) { return a.get() == static_cast<T*>(nullptr); }
    constexpr friend auto operator<=>(Box const& a, nullptr_t) { return a.get() <=> static_cast<T*>(nullptr); }

    T* m_pointer { nullptr };
};

template<typename T, typename... Args>
requires(!concepts::LanguageArray<T> && concepts::ConstructibleFrom<T, Args...>)
constexpr auto try_box(Args&&... args) {
    return platform::DefaultFallibleAllocator<T>().allocate(1) % [&](container::Allocation<T> result) {
        util::construct_at(result.data, util::forward<Args>(args)...);
        return Box<T>(result.data);
    };
}

template<typename T, typename... Args>
requires(!concepts::LanguageArray<T> && concepts::ConstructibleFrom<T, Args...>)
constexpr auto make_box(Args&&... args) {
    return *try_box<T>(util::forward<Args>(args)...);
}
}
