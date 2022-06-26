#pragma once

#include <liim/character_type.h>
#include <liim/container/hash.h>
#include <liim/option.h>
#include <liim/traits.h>
#include <liim/utilities.h>
#include <liim/vector.h>
#include <string.h>

namespace LIIM {
enum SplitMethod { KeepEmpty, RemoveEmpty };

class StringView {
public:
    constexpr StringView() : m_data(nullptr), m_size(0) {}
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
    constexpr bool operator==(const char* c_string) const { return *this == StringView(c_string); }

    constexpr size_t size() const { return m_size; }
    constexpr bool empty() const { return size() == 0; }
    constexpr const char* data() const { return m_data; }

    constexpr const char* begin() const { return m_data; }
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
    constexpr StringView substring(size_t count) const {
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

    constexpr Option<size_t> index_of(char c) const {
        for (size_t i = 0; i < size(); i++) {
            if (char_at(i) == c) {
                return { i };
            }
        }

        return {};
    }

    constexpr Option<size_t> last_index_of(char c) const {
        for (size_t i = size(); i > 0; i--) {
            if (char_at(i - 1) == c) {
                return { i - 1 };
            }
        }

        return {};
    }

    Vector<StringView> split(char c, SplitMethod split_method = SplitMethod::RemoveEmpty) const { return split({ &c, 1 }, split_method); }

    Vector<StringView> split(StringView characters, SplitMethod split_method = SplitMethod::RemoveEmpty) const {
        if (empty()) {
            return {};
        }

        auto ret = Vector<StringView> {};
        size_t word_start = 0;
        size_t index = 0;

        auto maybe_add_substring = [&] {
            auto substring_length = index - word_start;
            if (substring_length > 0 || split_method == SplitMethod::KeepEmpty) {
                ret.add(substring(word_start, substring_length));
            }
        };

        for (; index < size(); index++) {
            if (characters.index_of(char_at(index))) {
                maybe_add_substring();
                word_start = index + 1;
            }
        }

        maybe_add_substring();
        return ret;
    }

private:
    const char* m_data;
    size_t m_size;
};

constexpr Option<size_t> parse_number(const StringView& view) {
    if (view.empty()) {
        return {};
    }

    // FIXME: handle overlow
    size_t value = 0;
    for (auto c : view) {
        if (!is_digit(c)) {
            return {};
        }
        value *= 10;
        value += c - '0';
    }
    return value;
}

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

namespace Container::Hash {
    template<>
    struct HashFunction<StringView> {
        constexpr static void hash(Hasher& hasher, StringView string) { hasher.add({ string.data(), string.size() }); }

        using Matches = Tuple<const char*, String>;
    };
}
}

constexpr LIIM::StringView operator""sv(const char* data, size_t size) {
    return { data, size };
}

using LIIM::parse_number;
using LIIM::SplitMethod;
using LIIM::StringView;
