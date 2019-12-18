#pragma once

#include <liim/traits.h>
#include <string.h>

namespace LIIM {

class StringView {
public:
    StringView(const char* str) : m_start(str), m_end(str + strlen(str) - 1) {}
    StringView(const char* start, const char* end) : m_start(start), m_end(end) {}
    StringView(const StringView& other) : m_start(other.start()), m_end(other.end()) {}

    ~StringView() {
        m_start = nullptr;
        m_end = nullptr;
    }

    int size() const { return m_end - m_start + 1; }

    const char* start() const { return m_start; }
    const char* end() const { return m_end; }

    bool operator==(const StringView& other) const {
        if (other.size() != this->size()) {
            return false;
        }

        return memcmp(other.start(), this->start(), this->size()) == 0;
    }

    bool operator!=(const StringView& other) const { return !(*this == other); }

private:
    const char* m_start;
    const char* m_end;
};

template<> struct Traits<StringView> {
    static constexpr bool is_simple() { return false; }
    static unsigned int hash(const StringView& s) {
        unsigned int v = 0;
        for (int i = 0; i < s.size(); i++) {
            v += ~s.start()[i];
            v ^= s.start()[i];
        }

        return v;
    }
};

}

using LIIM::StringView;