#pragma once

#include <di/function/invoke.h>
#include <di/util/address_of.h>
#include <di/vocab/optional/get_value.h>
#include <di/vocab/optional/is_nullopt.h>
#include <di/vocab/optional/nullopt.h>
#include <di/vocab/optional/set_nullopt.h>
#include <di/vocab/optional/set_value.h>

namespace di::util {
template<typename T>
class ReferenceWrapper {
private:
    template<typename U>
    constexpr static T* get_address(U& value) {
        return &value;
    }

    // Prevent creating reference wrapper's from rvalue (tempory) references.
    template<typename U>
    constexpr static void get_address(U&&) = delete;

public:
    using Value = T;

    constexpr explicit ReferenceWrapper(vocab::NullOpt) {}

    template<typename U>
    requires(requires { get_address(util::declval<U>()); } && !concepts::ReferenceWrapper<meta::Decay<U>>)
    constexpr ReferenceWrapper(U&& value) : m_pointer(get_address(value)) {}

    constexpr ReferenceWrapper(ReferenceWrapper const&) = default;
    constexpr ReferenceWrapper& operator=(ReferenceWrapper const&) = default;

    constexpr operator T&() const { return get(); }
    constexpr T& get() const { return *m_pointer; }

    template<typename... Args>
    requires(concepts::Invocable<T&, Args...>)
    constexpr meta::InvokeResult<T&, Args...> operator()(Args&&... args) const {
        return function::invoke(get(), util::forward<Args>(args)...);
    }

private:
    // Implement di::vocab::OptionalStorage.
    constexpr friend bool tag_invoke(types::Tag<vocab::is_nullopt>, ReferenceWrapper const& self) {
        return !self.m_pointer;
    }
    constexpr friend T& tag_invoke(types::Tag<vocab::get_value>, ReferenceWrapper const& self) {
        return *self.m_pointer;
    }
    constexpr friend void tag_invoke(types::Tag<vocab::set_nullopt>, ReferenceWrapper& self) {
        self.m_pointer = nullptr;
    }
    constexpr friend void tag_invoke(types::Tag<vocab::set_value>, ReferenceWrapper& self, T& value) {
        self.m_pointer = util::address_of(value);
    }

    T* m_pointer { nullptr };
};

template<typename T>
ReferenceWrapper(T&) -> ReferenceWrapper<T>;

template<typename T>
constexpr ReferenceWrapper<T> ref(T& value) {
    return ReferenceWrapper<T>(value);
}

template<typename T>
constexpr ReferenceWrapper<T const> cref(T const& value) {
    return ReferenceWrapper<T const>(value);
}

// Prevent reference wrapper's from wrapping reference wrappers.
template<typename T>
constexpr ReferenceWrapper<T> ref(ReferenceWrapper<T> value) {
    return ReferenceWrapper<T>(value.get());
}

template<typename T>
constexpr ReferenceWrapper<T const> cref(ReferenceWrapper<T> value) {
    return ReferenceWrapper<T const>(value.get());
}

// Prevent construction of reference wrapper's from temporaries.
template<typename T>
constexpr void ref(const T&& value) = delete;

template<typename T>
constexpr void cref(const T&& value) = delete;
}
