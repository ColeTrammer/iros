#pragma once

#include <liim/option.h>
#include <liim/string_view.h>
#include <liim/utilities.h>
#include <stdint.h>

namespace LIIM {
class Utf8ViewIterator;

class Utf8View {
public:
    static constexpr uint32_t replacement_character = 0xFFFDU;

    constexpr Utf8View() = default;
    constexpr explicit Utf8View(const char* data) : m_data { data }, m_size_in_bytes { 0 } {
        while (*data++) {
            m_size_in_bytes++;
        }
    }
    constexpr explicit Utf8View(const char* data, size_t size_in_bytes) : m_data { data }, m_size_in_bytes { size_in_bytes } {}
    constexpr explicit Utf8View(StringView view) : m_data { view.data() }, m_size_in_bytes { view.size() } {}

    constexpr const char* data() const { return m_data; }
    constexpr size_t size_in_bytes() const { return m_size_in_bytes; }
    constexpr bool empty() const { return size_in_bytes() == 0; }
    constexpr Option<char> byte_at(size_t byte_offset) const {
        return byte_offset < size_in_bytes() ? Option<char> { data()[byte_offset] } : Option<char> {};
    }

    constexpr Utf8ViewIterator begin() const;
    constexpr Utf8ViewIterator end() const;
    constexpr Utf8ViewIterator iterator_at_byte_offset(size_t byte_offset) const;

    constexpr bool validate() const;

private:
    const char* m_data { nullptr };
    size_t m_size_in_bytes { 0 };
};

class Utf8ViewIterator {
public:
    struct CodePointInfo {
        Option<uint32_t> codepoint;
        size_t bytes_used;
    };

    constexpr CodePointInfo current_code_point_info() const;
    constexpr Option<uint32_t> current_code_point() const { return current_code_point_info().codepoint; }
    constexpr void advance() { m_byte_offset += current_code_point_info().bytes_used; }

    constexpr uint32_t operator*() const { return current_code_point().value_or(Utf8View::replacement_character); }
    constexpr void operator++() { advance(); }
    constexpr Utf8ViewIterator operator++(int) {
        auto save = *this;
        advance();
        return save;
    }

    constexpr size_t byte_offset() const { return m_byte_offset; }

    constexpr bool operator==(const Utf8ViewIterator& other) const {
        return this->m_view.data() == other.m_view.data() && this->m_view.size_in_bytes() == other.m_view.size_in_bytes() &&
               this->byte_offset() == other.byte_offset();
    }

private:
    friend class Utf8View;

    constexpr Utf8ViewIterator(Utf8View view, size_t byte_offset) : m_view { view }, m_byte_offset { byte_offset } {}

    Utf8View m_view;
    size_t m_byte_offset { 0 };
};

constexpr Utf8ViewIterator Utf8View::begin() const {
    return Utf8ViewIterator { *this, 0 };
}
constexpr Utf8ViewIterator Utf8View::end() const {
    return Utf8ViewIterator { *this, size_in_bytes() };
}
constexpr Utf8ViewIterator Utf8View::iterator_at_byte_offset(size_t byte_offset) const {
    assert(byte_offset <= size_in_bytes());
    return Utf8ViewIterator { *this, byte_offset };
}

constexpr bool Utf8View::validate() const {
    for (auto iter = begin(); iter != end(); ++iter) {
        if (!iter.current_code_point()) {
            return false;
        }
    }
    return true;
}

constexpr Utf8ViewIterator::CodePointInfo Utf8ViewIterator::current_code_point_info() const {
    auto first_byte_value = [](uint8_t byte, size_t byte_count) -> uint32_t {
        switch (byte_count) {
            case 1:
                return byte;
            case 2:
                return byte & 0b0001'1111;
            case 3:
                return byte & 0b0000'1111;
            case 4:
                return byte & 0b0000'0111;
            default:
                return 0;
        }
    };

    auto latter_byte_value = [](uint8_t byte) -> uint32_t {
        return byte & 0b0011'1111;
    };

    auto valid_byte = [](uint8_t byte, uint8_t first_byte, size_t byte_index, size_t expected_byte_count) -> bool {
        if (byte == 0xC0 || byte == 0xC1 || byte >= 0xF5) {
            return false;
        }

        switch (byte_index) {
            case 0:
                return true;
            case 1:
                if (first_byte == 0xE0 && expected_byte_count == 3) {
                    return byte >= 0xA0 && byte <= 0xBF;
                }
                if (first_byte == 0xED && expected_byte_count == 3) {
                    return byte >= 0x80 && byte <= 0x9F;
                }
                if (first_byte == 0xF0 && expected_byte_count == 4) {
                    return byte >= 0x90 && byte <= 0xBF;
                }
                if (first_byte == 0xF4 && expected_byte_count == 4) {
                    return byte >= 0x80 && byte <= 0x8F;
                }
                [[fallthrough]];
            case 2:
            case 3:
                return byte >= 0x80 && byte <= 0xBF;
            default:
                return false;
        }
    };

    auto expected_byte_count = [](uint8_t first_byte) -> Option<size_t> {
        if (first_byte == 0xC0 || first_byte == 0xC1 || first_byte >= 0xF5) {
            return {};
        }

        if ((first_byte & 0b1000'0000) == 0) {
            return 1;
        }

        if ((first_byte & 0b1110'0000) == 0b1100'0000) {
            return 2;
        }

        if ((first_byte & 0b1111'0000) == 0b1110'0000) {
            return 3;
        }

        if ((first_byte & 0b1111'1000) == 0b1111'0000) {
            return 4;
        }

        return {};
    };

    size_t bytes_used = 0;
    auto first_byte = m_view.byte_at(m_byte_offset + bytes_used);
    if (!first_byte) {
        return { {}, 0 };
    }

    auto maybe_byte_count = expected_byte_count(*first_byte);
    if (!maybe_byte_count) {
        return { {}, 1 };
    }
    auto byte_count = *maybe_byte_count;

    auto value = first_byte_value(*first_byte, byte_count);
    for (bytes_used = 1; bytes_used < byte_count; bytes_used++) {
        auto byte = m_view.byte_at(m_byte_offset + bytes_used);
        if (!byte) {
            return { {}, bytes_used };
        }

        if (!valid_byte(*byte, *first_byte, bytes_used, byte_count)) {
            return { {}, bytes_used };
        }

        value <<= 6;
        value |= latter_byte_value(*byte);
    }
    return { value, bytes_used };
}
}

using LIIM::Utf8View;
