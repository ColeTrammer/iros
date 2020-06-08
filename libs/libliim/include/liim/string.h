#pragma once

#include <assert.h>
#include <ctype.h>
#include <liim/maybe.h>
#include <liim/pointers.h>
#include <liim/string_view.h>
#include <liim/traits.h>
#include <liim/utilities.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace LIIM {

#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

class String {
public:
    static SharedPtr<String> wrap_malloced_chars(char* chars);
    static __attribute__((format(printf, 1, 2))) String format(const char* format, ...);

    explicit String(char c);
    explicit String(const StringView& view);
    String(const char* chars = "");
    String(const String& other);
    String(String&& other);

    ~String() { clear(); }

    String& operator=(const String& other);
    String& operator=(String&& other);

    String& operator+=(const String& other) {
        insert(other, size());
        return *this;
    }

    bool operator==(const String& other) const { return strcmp(this->string(), other.string()) == 0; }
    bool operator!=(const String& other) const { return !(*this == other); }

    char& operator[](size_t index) { return string()[index]; }
    const char& operator[](size_t index) const { return string()[index]; };

    size_t size() const { return is_small() ? (m_string.small.size_and_flag >> 1) : m_string.large.size; }
    bool is_empty() const { return size() == 0; }

    size_t capacity() const { return is_small() ? sizeof(m_string.small.data) : (m_string.large.capacity & ~1); };

    char* begin() { return string(); }
    char* end() { return string() + size(); }

    const char* begin() const { return string(); }
    const char* end() const { return string() + size(); }

    const char* cbegin() const { return string(); }
    const char* cend() const { return string() + size(); }

    char* string() { return is_small() ? m_string.small.data : m_string.large.data; }
    const char* string() const { return is_small() ? m_string.small.data : m_string.large.data; }

    StringView view() const { return StringView(string(), string() + size() - 1); }

    void insert(const StringView& view, size_t position);
    void insert(const String& string, size_t position) { insert(string.view(), position); }
    void insert(char c, size_t position) { insert(String(c), position); }

    void clear();
    void remove_index(size_t index);

    String& to_upper_case();
    String& to_lower_case();
    String& to_title_case();

    Maybe<size_t> index_of(char c) const;

    void swap(String& other);

private:
    bool is_small() const { return m_string.small.size_and_flag & 1; }

    char* allocate_string(size_t capacity) { return static_cast<char*>(malloc(capacity)); }
    size_t round_up_capacity(size_t requested_capacity);
    void set_capacity(size_t capacity) { m_string.large.capacity = capacity; }
    void set_size(size_t size);

    union {
        struct {
            size_t capacity;
            size_t size;
            char* data;
        } large;
        struct {
            unsigned char size_and_flag;
            char data[sizeof(size_t) + sizeof(size_t) + sizeof(char*) - 1];
        } small;
    } m_string;
};

inline SharedPtr<String> String::wrap_malloced_chars(char* chars) {
    auto ret = make_shared<String>();

    size_t len = strlen(chars);
    ret->set_capacity(ret->round_up_capacity(len + 1));
    ret->set_size(len);
    ret->m_string.large.data = chars;
    return ret;
}

inline String String::format(const char* format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 2047, format, args);
    va_end(args);
    return String(buffer);
}

inline String::String(char c) {
    m_string.small.size_and_flag = 0b11;
    string()[0] = c;
    string()[1] = '\0';
}

inline String::String(const StringView& view) {
    size_t needed_capacity = view.size() + 1;
    if (needed_capacity >= sizeof(m_string.small.data)) {
        set_capacity(round_up_capacity(needed_capacity));
        m_string.large.data = allocate_string(capacity());
    } else {
        m_string.small.size_and_flag = 1;
    }

    memcpy(string(), view.start(), view.size());
    string()[view.size()] = '\0';

    set_size(needed_capacity - 1);
}

inline String::String(const char* chars) : String(StringView(chars)) {}

inline String::String(const String& other) : String(other.string()) {}

inline String::String(String&& other) {
    if (other.is_small()) {
        this->m_string.small.size_and_flag = other.m_string.small.size_and_flag;
        memcpy(this->string(), other.string(), other.size() + 1);
    } else {
        this->m_string.large = other.m_string.large;
        other.m_string.large.data = nullptr;
    }
    other.clear();
}

inline String& String::operator=(const String& other) {
    if (this != &other) {
        String temp(other);
        swap(temp);
    }
    return *this;
}

inline String& String::operator=(String&& other) {
    if (this != &other) {
        String temp(move(other));
        swap(temp);
    }
    return *this;
}

inline void String::insert(const StringView& to_insert, size_t position) {
    assert(position <= size());
    size_t new_size = size() + to_insert.size();
    if (new_size + 1 > capacity()) {
        const bool was_small = is_small();
        size_t new_capacity = round_up_capacity(new_size + 1);
        char* allocated_string = allocate_string(new_capacity);

        memcpy(allocated_string, string(), position);
        memcpy(allocated_string + position, to_insert.start(), to_insert.size());
        memcpy(allocated_string + position + to_insert.size(), string() + position, size() + 1 - position);

        set_capacity(new_capacity);
        set_size(new_size);
        if (!was_small) {
            free(m_string.large.data);
        }
        m_string.large.data = allocated_string;
        return;
    }

    memmove(string() + position + to_insert.size(), string() + position, size() + 1 - position);
    memcpy(string() + position, to_insert.start(), to_insert.size());
    set_size(new_size);
}

inline void String::clear() {
    if (!is_small()) {
        free(m_string.large.data);
    }

    m_string.small.size_and_flag = 1;
    m_string.small.data[0] = '\0';
}

inline void String::remove_index(size_t index) {
    assert(index < size());
    memmove(string() + index, string() + index + 1, size() - index);
    set_size(size() - 1);
}

inline String& String::to_upper_case() {
    for (size_t i = 0; i < size(); i++) {
        if (islower(string()[i])) {
            string()[i] = _toupper(string()[i]);
        }
    }

    return *this;
}

inline String& String::to_lower_case() {
    for (size_t i = 0; i < size(); i++) {
        if (isupper(string()[i])) {
            string()[i] = _tolower(string()[i]);
        }
    }

    return *this;
}

inline String& String::to_title_case() {
    for (size_t i = 0; i < size(); i++) {
        if (i == 0) {
            string()[i] = toupper(string()[i]);
            continue;
        }

        if (string()[i] == '_') {
            string()[i + 1] = toupper(string()[i + 1]);
            memmove(string() + i, string() + i + 1, size() - i - 1);
        } else {
            string()[i] = tolower(string()[i]);
        }
    }

    return *this;
}

inline Maybe<size_t> String::index_of(char c) const {
    const char* s = strchr(string(), c);
    if (s == NULL) {
        return {};
    }

    return { static_cast<size_t>(s - string()) };
}

inline void String::swap(String& other) {
    if (this == &other) {
        return;
    }

    if (this->is_small() && other.is_small()) {
        char temp_chars[sizeof(m_string.small.data)];
        memcpy(temp_chars, other.string(), other.size() + 1);
        unsigned char other_size_and_flags = other.m_string.small.size_and_flag;

        other.m_string.small.size_and_flag = this->m_string.small.size_and_flag;
        memcpy(other.string(), this->string(), this->size() + 1);

        this->m_string.small.size_and_flag = other_size_and_flags;
        memcpy(this->string(), temp_chars, this->size() + 1);
        return;
    }

    if (this->is_small() && !other.is_small()) {
        other.swap(*this);
        return;
    }

    if (!this->is_small() && other.is_small()) {
        char temp_chars[sizeof(m_string.small.data)];
        memcpy(temp_chars, other.string(), other.size() + 1);
        unsigned char other_size_and_flags = other.m_string.small.size_and_flag;

        other.m_string.large = this->m_string.large;

        this->m_string.small.size_and_flag = other_size_and_flags;
        memcpy(this->string(), temp_chars, this->size() + 1);
        return;
    }

    LIIM::swap(this->m_string.large.capacity, other.m_string.large.capacity);
    LIIM::swap(this->m_string.large.size, other.m_string.large.size);
    LIIM::swap(this->m_string.large.data, other.m_string.large.data);
}

inline size_t String::round_up_capacity(size_t requested_capacity) {
    size_t capacity = 64;
    while (capacity < requested_capacity) {
        capacity <<= 1;
    }
    return capacity;
}

inline void String::set_size(size_t size) {
    assert(size < capacity());
    if (is_small()) {
        m_string.small.size_and_flag = (size << 1) | 1;
    } else {
        m_string.large.size = size;
    }
}

#pragma GCC diagnostic pop

template<typename T>
void swap(String& a, String& b) {
    a.swap(b);
}

template<>
struct Traits<String> {
    static constexpr bool is_simple() { return false; }
    static unsigned int hash(const String& s) {
        unsigned int v = 0;
        for (size_t i = 0; i < s.size(); i++) {
            v += ~s[i];
            v ^= s[i];
        }

        return v;
    }
};
}

using LIIM::String;
