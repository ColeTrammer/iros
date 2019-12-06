#pragma once

#include <liim/traits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace LIIM {

class String {
public:
    String() {}

    String(const char* chars) : m_size(strlen(chars)), m_string(strdup(chars)) {}

    String(const String& other) : m_size(other.size()), m_string(strdup(other.string())) {}

    ~String() { free(m_string); }

    int size() const { return m_size; }

    char* string() { return m_string; }
    const char* string() const { return m_string; }

    char& operator[](int index) { return string()[index]; }

    const char& operator[](int index) const { return string()[index]; }

    bool operator==(const String& other) { return strcmp(string(), other.string()) == 0; }

    bool operator!() const { return !m_string; }
    operator bool() const { return !!m_string; }

    bool is_empty() const { return size() == 0; }

private:
    int m_size { 0 };
    char* m_string { nullptr };
};

template<> struct Traits<String> {
    static constexpr bool is_simple() { return false; }
    static unsigned int hash(const String& s) {
        unsigned int v = 0;
        for (int i = 0; i < s.size(); i++) {
            v += ~s[i];
            v ^= s[i];
        }

        return v;
    }
};

}

using LIIM::String;