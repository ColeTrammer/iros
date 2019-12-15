#pragma once

#include <memory>
#include <stdint.h>

namespace LIIM {

template<typename T> struct Traits {
    static constexpr bool is_simple() { return false; }
};

template<> struct Traits<bool> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const bool& obj) { return static_cast<unsigned int>(obj); };
};

template<> struct Traits<uint8_t> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const uint8_t& obj) { return static_cast<unsigned int>(obj); };
};

template<> struct Traits<uint16_t> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const uint16_t& obj) { return static_cast<unsigned int>(obj); };
};

template<> struct Traits<uint32_t> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const uint32_t& obj) { return static_cast<unsigned int>(obj); };
};

template<> struct Traits<uint64_t> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const uint64_t& obj) { return static_cast<unsigned int>(obj) + static_cast<unsigned int>(obj >> 32); };
};

template<> struct Traits<char> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const int8_t& obj) { return static_cast<unsigned int>(obj); };
};

template<> struct Traits<int8_t> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const int8_t& obj) { return static_cast<unsigned int>(obj); };
};

template<> struct Traits<int16_t> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const int16_t& obj) { return static_cast<unsigned int>(obj); };
};

template<> struct Traits<int32_t> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const int32_t& obj) { return static_cast<unsigned int>(obj); };
};

template<> struct Traits<int64_t> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const int64_t& obj) { return static_cast<unsigned int>(obj) + static_cast<unsigned int>(obj >> 32); };
};

template<typename U> struct Traits<std::shared_ptr<U>> {
    static constexpr bool is_simple() { return false; }
    static unsigned int hash(const std::shared_ptr<U>& obj) {
        return static_cast<unsigned int>((uintptr_t) obj.get()) + static_cast<unsigned int>((uintptr_t) obj.get() >> 32);
    }
};

template<typename U> struct Traits<U*> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const U*& obj) {
        return static_cast<unsigned int>((uintptr_t) obj) + static_cast<unsigned int>((uintptr_t) obj >> 32);
    };
};

}