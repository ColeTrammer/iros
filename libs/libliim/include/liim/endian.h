#pragma once

#include <liim/format.h>
#include <liim/utilities.h>

namespace LIIM {
template<Integral T>
constexpr T host_to_big_endian(T value) {
    if constexpr (sizeof(T) == 2) {
        return __builtin_bswap16(value);
    } else if constexpr (sizeof(T) == 4) {
        return __builtin_bswap32(value);
    } else if constexpr (sizeof(T) == 8) {
        return __builtin_bswap64(value);
    }
    return value;
}

template<Integral T>
constexpr T host_to_little_endian(T value) {
    return value;
}

template<Integral T>
constexpr T big_endian_to_host(T value) {
    if constexpr (sizeof(T) == 2) {
        return __builtin_bswap16(value);
    } else if constexpr (sizeof(T) == 4) {
        return __builtin_bswap32(value);
    } else if constexpr (sizeof(T) == 8) {
        return __builtin_bswap64(value);
    }
    return value;
}

template<Integral T>
constexpr T little_endian_to_host(T value) {
    return value;
}

template<Integral T>
class [[gnu::packed]] BigEndian {
public:
    constexpr explicit BigEndian(T value) : m_big_endian_value(host_to_big_endian(value)) {}

    constexpr operator T() const { return big_endian_to_host(m_big_endian_value); }

private:
    T m_big_endian_value { 0 };
};

template<Integral T>
class [[gnu::packed]] LittleEndian {
public:
    constexpr explicit LittleEndian(T value) : m_little_endian_value(host_to_little_endian(value)) {}

    constexpr operator T() const { return little_endian_to_host(m_little_endian_value); }

private:
    T m_little_endian_value { 0 };
};
}

namespace LIIM::Format {
template<Integral T>
struct Formatter<BigEndian<T>> : Formatter<T> {};

template<Integral T>
struct Formatter<LittleEndian<T>> : Formatter<T> {};
}

using LIIM::BigEndian;
using LIIM::LittleEndian;
