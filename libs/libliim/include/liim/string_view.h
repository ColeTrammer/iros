#pragma once

#include <liim/maybe.h>
#include <liim/traits.h>
#include <liim/utilities.h>
#include <liim/vector.h>
#include <string.h>

namespace LIIM {

class StringView {
public:
    StringView(const char* str) : m_start(str), m_end(str + strlen(str)) {}
    StringView(const char* start, const char* end) : m_start(start), m_end(end + 1) {}
    StringView(const StringView& other) : m_start(other.m_start), m_end(other.m_end) {}

    ~StringView() {
        m_start = nullptr;
        m_end = nullptr;
    }

    StringView& operator=(const StringView& other) {
        this->m_start = other.start();
        this->m_end = other.m_end;
        return *this;
    }

    size_t size() const { return static_cast<size_t>(m_end - m_start); }
    bool empty() const { return size() == 0; }

    const char* start() const { return m_start; }
    const char* end() const { return m_end - 1; }

    Maybe<size_t> index_of(char c) const {
        for (size_t i = 0; i < size(); i++) {
            if (start()[i] == c) {
                return { i };
            }
        }

        return {};
    }

    bool starts_with(const StringView& other) const {
        if (this->size() < other.size()) {
            return false;
        }

        for (size_t i = 0; i < other.size(); i++) {
            if (this->start()[i] != other.start()[i]) {
                return false;
            }
        }

        return true;
    }

    bool ends_with(const StringView& other) const {
        if (this->size() < other.size()) {
            return false;
        }

        for (size_t i = 0; i < other.size(); i++) {
            if (this->start()[i + this->size() - other.size()] != other.start()[i]) {
                return false;
            }
        }

        return true;
    }

    bool operator==(const StringView& other) const {
        if (other.size() != this->size()) {
            return false;
        }

        return memcmp(other.start(), this->start(), this->size()) == 0;
    }

    Vector<StringView> split(char c) const {
        Vector<StringView> ret;

        const char* word_start = nullptr;
        for (size_t i = 0; i < size(); i++) {
            if (!word_start && start()[i] != c) {
                word_start = start() + i;
            }
            if (word_start && start()[i] == c) {
                ret.add({ word_start, start() + i - 1 });
                word_start = nullptr;
            }
        }

        if (word_start) {
            ret.add({ word_start, end() });
        }
        return ret;
    }

    const char& operator[](size_t index) const { return m_start[index]; }

    bool operator!=(const StringView& other) const { return !(*this == other); }

private:
    const char* m_start;
    const char* m_end;
};

template<>
struct Traits<StringView> {
    static constexpr bool is_simple() { return false; }
    static unsigned int hash(const StringView& s) {
        unsigned int v = 0;
        for (size_t i = 0; i < s.size(); i++) {
            v += ~s.start()[i];
            v ^= s.start()[i];
        }

        return v;
    }
};

}

using LIIM::StringView;
