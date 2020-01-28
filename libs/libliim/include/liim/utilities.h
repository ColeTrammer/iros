#pragma once

#include <new>

namespace std {
typedef decltype(nullptr) nullptr_t;
}

namespace LIIM {

template<typename T> struct Identity { typedef T Type; };

template<typename T> inline constexpr T&& forward(typename Identity<T>::Type& param) {
    return static_cast<T&&>(param);
}

template<typename T> inline T&& move(T&& arg) {
    return static_cast<T&&>(arg);
}

template<typename T> void swap(T& a, T& b) {
    T temp(a);
    a = b;
    b = temp;
}

}

using LIIM::move;