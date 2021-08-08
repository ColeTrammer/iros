#pragma once

#include <stdint.h>

namespace LIIM {

template<typename T>
struct Traits {
    static constexpr bool is_simple() { return false; }
};

template<>
struct Traits<bool> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const bool& obj) { return static_cast<unsigned int>(obj); };
};

template<>
struct Traits<unsigned char> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const unsigned char& obj) { return static_cast<unsigned int>(obj); };
};

template<>
struct Traits<unsigned short> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const unsigned short& obj) { return static_cast<unsigned int>(obj); };
};

template<>
struct Traits<unsigned int> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const unsigned int& obj) { return static_cast<unsigned int>(obj); };
};

template<>
struct Traits<unsigned long> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const unsigned long& obj) { return static_cast<unsigned int>(obj) + static_cast<unsigned int>(obj >> 32); };
};

template<>
struct Traits<unsigned long long> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const unsigned long long& obj) {
        return static_cast<unsigned int>(obj) + static_cast<unsigned int>(obj >> 32);
    };
};

template<>
struct Traits<char> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const char& obj) { return static_cast<unsigned int>(obj); };
};

template<>
struct Traits<short> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const short& obj) { return static_cast<unsigned int>(obj); };
};

template<>
struct Traits<int> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const unsigned int& obj) { return static_cast<unsigned int>(obj); };
};

template<>
struct Traits<long> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const long& obj) { return static_cast<unsigned int>(obj) + static_cast<unsigned int>(obj >> 32); };
};

template<>
struct Traits<long long> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const long long& obj) { return static_cast<unsigned int>(obj) + static_cast<unsigned int>(obj >> 32); };
};

template<typename U>
struct Traits<U*> {
    static constexpr bool is_simple() { return true; }
    static unsigned int hash(const U* obj) {
        return static_cast<unsigned int>((uintptr_t) obj) + static_cast<unsigned int>((uintptr_t) obj >> 32);
    };
};

}
