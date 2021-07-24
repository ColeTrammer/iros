#pragma once

#include <liim/maybe.h>
#include <liim/traits.h>
#include <liim/utilities.h>
#include <liim/vector.h>
#include <string.h>

namespace LIIM {

class StringView {
public:
    constexpr StringView(const char* str) : m_data(str), m_size(0) {
        while (*str++) {
            m_size++;
        }
    }
    constexpr StringView(const char* data, const char* last) : m_data(data), m_size(static_cast<size_t>(last - data)) {}
    constexpr StringView(const char* data, size_t size) : m_data(data), m_size(size) {}

    constexpr bool operator==(const StringView& other) const {
        if (other.size() != this->size()) {
            return false;
        }

        for (size_t i = 0; i < size(); i++) {
            if (this->char_at(i) != other.char_at(i)) {
                return false;
            }
        }
        return true;
    }
    constexpr bool operator!=(const StringView& other) const { return !(*this == other); }

    constexpr size_t size() const { return m_size; }
    constexpr bool empty() const { return size() == 0; }
    constexpr const char* data() const { return m_data; }

    constexpr const char* start() const { return m_data; }
    constexpr const char* end() const { return m_data + m_size; }

    constexpr const char& operator[](size_t index) const { return char_at(index); }
    constexpr const char& char_at(size_t index) const {
        assert(index < size());
        return m_data[index];
    }

    constexpr const char& first() const { return char_at(0); }
    constexpr const char& last() const { return char_at(size() - 1); }

    constexpr StringView first(size_t count) const {
        assert(count <= size());
        return { data(), count };
    }
    constexpr StringView last(size_t count) const {
        assert(count <= size());
        return { data() + (size() - count), count };
    }
    constexpr StringView after(size_t count) const {
        assert(count <= size());
        return { data() + count, size() - count };
    }
    constexpr StringView substring(size_t index, size_t count) const {
        assert(index + count <= size());
        return { data() + index, count };
    }

    constexpr bool starts_with(const StringView& other) const {
        if (this->size() < other.size()) {
            return false;
        }

        for (size_t i = 0; i < other.size(); i++) {
            if (this->char_at(i) != other.char_at(i)) {
                return false;
            }
        }

        return true;
    }

    constexpr bool ends_with(const StringView& other) const {
        if (this->size() < other.size()) {
            return false;
        }

        for (size_t i = 0; i < other.size(); i++) {
            if (this->char_at(i + this->size() - other.size()) != other.char_at(i)) {
                return false;
            }
        }

        return true;
    }

    Maybe<size_t> index_of(char c) const {
        for (size_t i = 0; i < size(); i++) {
            if (char_at(i) == c) {
                return { i };
            }
        }

        return {};
    }

    Vector<StringView> split(char c) const {
        Vector<StringView> ret;

        const char* word_start = nullptr;
        for (size_t i = 0; i < size(); i++) {
            if (!word_start && char_at(i) != c) {
                word_start = m_data + i;
            }
            if (word_start && char_at(i) == c) {
                ret.add({ word_start, data() + i });
                word_start = nullptr;
            }
        }

        if (word_start) {
            ret.add({ word_start, data() + size() });
        }
        return ret;
    }

private:
    const char* m_data;
    size_t m_size;
};

template<>
struct Traits<StringView> {
    static constexpr bool is_simple() { return false; }
    static unsigned int hash(const StringView& s) {
        unsigned int v = 0;
        for (size_t i = 0; i < s.size(); i++) {
            v += ~s[i];
            v ^= s[i];
        }

        return v;
    }
};
}

constexpr LIIM::StringView operator""sv(const char* data, size_t size) {
    return { data, size };
}

using LIIM::StringView;
