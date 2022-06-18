#pragma once

#include <liim/error/common_result.h>
#include <liim/utilities.h>

namespace LIIM {
template<typename T, typename U>
concept OperatorAssignableFrom = requires(T& lvalue, U&& rvalue) {
    { lvalue = static_cast<U&&>(rvalue) } -> SameAs<T&>;
};

template<typename T>
concept Copyable = ConstructibleFrom<T, const T&>;

template<typename T>
concept Moveable = ConstructibleFrom<T, T&&> && OperatorAssignableFrom<T, T&&>;

template<typename T>
concept MemberCloneable = Moveable<T> && requires(const T& clvalue) {
    { clvalue.clone() } -> SameAs<T>;
};

template<typename T>
concept Cloneable = Copyable<T> || MemberCloneable<T>;

template<typename T>
concept FalliblyCloneable = !Cloneable<T> && Moveable<T> && requires(const T& clvalue) {
    { clvalue.clone() } -> ResultOf<T>;
};

template<typename T, typename... Args>
concept MemberCreateableFrom = Moveable<T> && requires(Args&&... args) {
    { T::create(static_cast<Args&&>(args)...) } -> SameAs<T>;
};

template<typename T, typename... Args>
concept FalliblyMemberCreateableFrom = Moveable<T> && requires(Args&&... args) {
    { T::create(static_cast<Args&&>(args)...) } -> ResultOf<T>;
};

template<typename T, typename... Args>
concept CreateableFrom = ConstructibleFrom<T, Args...> || MemberCreateableFrom<T, Args...>;

template<typename T, typename... Args>
concept FalliblyCreateableFrom =
    !CreateableFrom<T, Args...> && (CreateableFrom<T, UnwrapResult<Args>...> || FalliblyMemberCreateableFrom<T, UnwrapResult<Args>...>);

template<typename T, typename... Args>
concept MemberAssignableFrom = requires(T& lvalue, Args&&... args) {
    { lvalue.assign(static_cast<Args&&>(args)...) } -> SameAs<T&>;
};

template<typename T, typename... Args>
concept FalliblyMemberAssignableFrom = requires(T& lvalue, Args&&... args) {
    { lvalue.assign(static_cast<Args&&>(args)...) } -> ResultOf<T&>;
};

template<typename T, typename U>
concept AssignableFrom = OperatorAssignableFrom<T, U> || MemberAssignableFrom<T, U> ||(Moveable<T>&& CreateableFrom<T, U>);

template<typename T, typename U>
concept FalliblyAssignableFrom = !AssignableFrom<T, U> && (FalliblyMemberAssignableFrom<T, U> || FalliblyCreateableFrom<T, U>);

template<typename T, typename... Args>
requires(CreateableFrom<T, Args...> || FalliblyCreateableFrom<T, Args...>) using CreateResult = decltype(create<T>(declval<Args&&>()...));

template<typename T, typename... Args>
constexpr void create_at(T* location, Args&&... args) requires(CreateableFrom<T, Args...>) {
    if constexpr (ConstructibleFrom<T, Args...>) {
        construct_at(location, forward<Args>(args)...);
    } else if constexpr (MemberCreateableFrom<T, Args...>) {
        construct_at(location, T::create(forward<Args>(args)...));
    }
}

template<typename T, typename... Args>
constexpr T create(Args&&... args) requires(CreateableFrom<T, Args...>) {
    if constexpr (ConstructibleFrom<T, Args...>) {
        return T(forward<Args>(args)...);
    } else if constexpr (CreateableFrom<T, Args...>) {
        return T::create(forward<Args>(args)...);
    }
}

template<typename T, typename... Args>
constexpr auto create(Args&&... args) requires(FalliblyMemberCreateableFrom<T, Args...>) {
    return T::create(args...);
}

template<typename T, typename U>
constexpr T& assign_to(T& lvalue, U&& rvalue) requires(AssignableFrom<T, U>) {
    if constexpr (OperatorAssignableFrom<T, U>) {
        return lvalue = forward<U>(rvalue);
    } else if constexpr (MemberAssignableFrom<T, U>) {
        return lvalue.assign(forward<U>(rvalue));
    } else if constexpr (Moveable<T> && CreateableFrom<T, U>) {
        return lvalue = create<T>(forward<U>(rvalue));
    }
}

template<typename T>
constexpr T clone(const T& value) requires(Cloneable<T>) {
    if constexpr (Copyable<T>) {
        return value;
    } else if constexpr (MemberCloneable<T>) {
        return value.clone();
    }
}
}

using LIIM::assign_to;
using LIIM::AssignableFrom;
using LIIM::clone;
using LIIM::Cloneable;
using LIIM::ConstructibleFrom;
using LIIM::Copyable;
using LIIM::create;
using LIIM::create_at;
using LIIM::CreateableFrom;
using LIIM::MemberAssignableFrom;
using LIIM::MemberCloneable;
