#pragma once

#include <assert.h>
#include <ctype.h>
#include <liim/string_view.h>
#include <liim/traits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace LIIM {

class String {
public:
    String() {}

    explicit String(char c) : m_size(2), m_string(reinterpret_cast<char*>(malloc(2))) {
        m_string[0] = c;
        m_string[1] = '\0';
    }

    String(const char* chars) : m_size(strlen(chars)), m_string(strdup(chars)) {}

    String(const String& other) : m_size(other.size()), m_string(strdup(other.string())) {}

    String(const StringView& view) : m_size(view.size()) {
        m_string = reinterpret_cast<char*>(malloc(size() + 1));
        memmove(m_string, view.start(), m_size);
        m_string[m_size] = '\0';
    }

    ~String() {
        free(m_string);
        m_string = nullptr;
        m_size = 0;
    }

    int size() const { return m_size; }
    void set_size(int size) { m_size = size; }

    char* string() { return m_string; }
    const char* string() const { return m_string; }

    char& operator[](int index) { return string()[index]; }

    const char& operator[](int index) const { return string()[index]; }

    String& operator=(const String& other) {
        free(m_string);

        this->m_string = strdup(other.string());
        assert(this->m_string);
        this->m_size = other.m_size;
        return *this;
    }

    bool operator==(const String& other) const { return strcmp(string(), other.string()) == 0; }
    bool operator!=(const String& other) const { return !(*this == other); }

    String& operator+=(const String& other) {
        m_size += other.size();
        m_string = reinterpret_cast<char*>(realloc(m_string, size() + 1));
        strcat(string(), other.string());
        return *this;
    }

    String& to_upper_case() {
        for (int i = 0; i < size(); i++) {
            if (islower(m_string[i])) {
                m_string[i] = _toupper(m_string[i]);
            }
        }

        return *this;
    }

    String& to_lower_case() {
        for (int i = 0; i < size(); i++) {
            if (isupper(m_string[i])) {
                m_string[i] = _tolower(m_string[i]);
            }
        }

        return *this;
    }

    String& to_title_case() {
        for (int i = 0; i < size(); i++) {
            if (i == 0) {
                m_string[i] = toupper(m_string[i]);
                continue;
            }

            if (m_string[i] == '_') {
                m_string[i + 1] = toupper(m_string[i + 1]);
                memmove(m_string + i, m_string + i + 1, size() - i - 1);
            } else {
                m_string[i] = tolower(m_string[i]);
            }
        }

        return *this;
    }

    bool operator!() const { return !m_string; }
    operator bool() const { return !!m_string; }

    bool is_empty() const { return size() == 0; }

    static __attribute__((format(printf, 1, 2))) String format(const char* format, ...) {
        char buffer[2048];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 2047, format, args);
        va_end(args);
        return String(buffer);
    }

    int index_of(char c) const {
        char* s = strchr(m_string, c);
        if (s == NULL) {
            return -1;
        }

        return s - m_string;
    }

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