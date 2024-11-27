#pragma once

#include <di/meta/language.h>

#ifndef DI_NO_USE_STD
#include <cstddef>
#else
namespace std {
enum class byte : unsigned char {};

template<di::concepts::Integral Int>
constexpr auto to_integer(std::byte value) noexcept -> Int {
    return Int(value);
}

constexpr auto operator<<(std::byte byte, di::concepts::Integral auto shift) noexcept -> std::byte {
    return std::byte(static_cast<unsigned char>(byte) << shift);
}

constexpr auto operator>>(std::byte byte, di::concepts::Integral auto shift) noexcept -> std::byte {
    return std::byte(static_cast<unsigned char>(byte) >> shift);
}

constexpr auto operator<<=(std::byte& byte, di::concepts::Integral auto shift) noexcept -> std::byte& {
    return byte = byte << shift;
}

constexpr auto operator>>=(std::byte& byte, di::concepts::Integral auto shift) noexcept -> std::byte& {
    return byte = byte >> shift;
}

constexpr auto operator|(std::byte a, std::byte b) noexcept -> std::byte {
    return std::byte(static_cast<unsigned char>(a) | static_cast<unsigned char>(b));
}

constexpr auto operator&(std::byte a, std::byte b) noexcept -> std::byte {
    return std::byte(static_cast<unsigned char>(a) & static_cast<unsigned char>(b));
}

constexpr auto operator^(std::byte a, std::byte b) noexcept -> std::byte {
    return std::byte(static_cast<unsigned char>(a) ^ static_cast<unsigned char>(b));
}

constexpr auto operator~(std::byte a) noexcept -> std::byte {
    return std::byte(~static_cast<unsigned char>(a));
}

constexpr auto operator|=(std::byte& a, std::byte b) noexcept -> std::byte& {
    return a = a | b;
}

constexpr auto operator&=(std::byte& a, std::byte b) noexcept -> std::byte& {
    return a = a & b;
}

constexpr auto operator^=(std::byte& a, std::byte b) noexcept -> std::byte& {
    return a = a ^ b;
}
}
#endif

namespace di::types {
using Byte = std::byte;
using byte = std::byte;
using std::to_integer;
}

namespace di {
using types::Byte;
using types::byte;
using types::to_integer;
}

#if !defined(DI_NO_GLOBALS) && !defined(DI_NO_GLOBAL_TYPES)
using di::byte;
#endif
