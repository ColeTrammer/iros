#pragma once

#include <di/meta/language.h>

#ifndef DI_NO_USE_STD
#include <cstddef>
#else
namespace std {
enum class byte : unsigned char {};

template<di::concepts::Integral Int>
constexpr Int to_integer(std::byte value) noexcept {
    return Int(value);
}

constexpr std::byte operator<<(std::byte byte, di::concepts::Integral auto shift) noexcept {
    return std::byte(static_cast<unsigned char>(byte) << shift);
}

constexpr std::byte operator>>(std::byte byte, di::concepts::Integral auto shift) noexcept {
    return std::byte(static_cast<unsigned char>(byte) >> shift);
}

constexpr std::byte& operator<<=(std::byte& byte, di::concepts::Integral auto shift) noexcept {
    return byte = byte << shift;
}

constexpr std::byte& operator>>=(std::byte& byte, di::concepts::Integral auto shift) noexcept {
    return byte = byte >> shift;
}

constexpr std::byte operator|(std::byte a, std::byte b) noexcept {
    return std::byte(static_cast<unsigned char>(a) | static_cast<unsigned char>(b));
}

constexpr std::byte operator&(std::byte a, std::byte b) noexcept {
    return std::byte(static_cast<unsigned char>(a) & static_cast<unsigned char>(b));
}

constexpr std::byte operator^(std::byte a, std::byte b) noexcept {
    return std::byte(static_cast<unsigned char>(a) ^ static_cast<unsigned char>(b));
}

constexpr std::byte operator~(std::byte a) noexcept {
    return std::byte(~static_cast<unsigned char>(a));
}

constexpr std::byte& operator|=(std::byte& a, std::byte b) noexcept {
    return a = a | b;
}

constexpr std::byte& operator&=(std::byte& a, std::byte b) noexcept {
    return a = a & b;
}

constexpr std::byte& operator^=(std::byte& a, std::byte b) noexcept {
    return a = a ^ b;
}
}
#endif

namespace di::types {
using Byte = std::byte;
using byte = std::byte;
using std::to_integer;
}
