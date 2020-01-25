#pragma once

namespace LIIM {

template<typename T> T&& forward(T&& param) {
    return static_cast<T&&>(param);
}

template<typename T> T&& move(T&& arg) {
    return static_cast<T&&>(arg);
}

template<typename T> void swap(T& a, T& b) {
    T temp(a);
    a = b;
    b = temp;
}

}

using LIIM::move;