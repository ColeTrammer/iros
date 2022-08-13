#pragma once

#include <liim/container/producer/iterator_container.h>
#include <liim/container/strings/encoding.h>
#include <liim/option.h>
#include <liim/span.h>

namespace LIIM::Container::Strings {
// NOTE: see https://www.unicode.org/versions/Unicode14.0.0/UnicodeStandard-14.0.pdf for details on the UTF-8 encoding.
//       In particular, section 3.9, table 3-6, and table 3-7.

namespace Utf8 {
    constexpr inline char32_t replacement_character = 0xFFFD;

    constexpr static bool is_start_of_one_byte_sequence(uint8_t byte) {
        return byte <= 0x7F;
    }

    constexpr static bool is_start_of_two_byte_sequence(uint8_t byte) {
        return byte >= 0xC2 && byte <= 0xDF;
    }

    constexpr static bool is_start_of_three_byte_sequence(uint8_t byte) {
        return byte >= 0xE0 && byte <= 0xEF;
    }

    constexpr static bool is_start_of_four_byte_sequence(uint8_t byte) {
        return byte >= 0xF0 && byte <= 0xF4;
    }

    constexpr static bool is_start_of_multi_byte_sequence(uint8_t byte) {
        return is_start_of_two_byte_sequence(byte) || is_start_of_three_byte_sequence(byte) || is_start_of_four_byte_sequence(byte);
    }

    constexpr static bool is_start_of_byte_sequence(uint8_t byte) {
        return is_start_of_one_byte_sequence(byte) || is_start_of_multi_byte_sequence(byte);
    }

    constexpr static bool is_valid_second_byte(uint8_t first_byte, uint8_t second_byte) {
        switch (first_byte) {
            case 0xE0:
                return second_byte >= 0xA0 && second_byte <= 0xBF;
            case 0xED:
                return second_byte >= 0x80 && second_byte <= 0x9F;
            case 0xF0:
                return second_byte >= 0x90 && second_byte <= 0xBF;
            case 0xF4:
                return second_byte >= 0x80 && second_byte <= 0x8F;
            default:
                return second_byte >= 0x80 && second_byte <= 0xBF;
        }
    }

    constexpr static bool is_valid_third_byte([[maybe_unused]] uint8_t first_byte, uint8_t third_byte) {
        return third_byte >= 0x80 && third_byte <= 0xBF;
    }

    constexpr static bool is_valid_fourth_byte([[maybe_unused]] uint8_t first_byte, uint8_t fourth_byte) {
        return fourth_byte >= 0x80 && fourth_byte <= 0xBF;
    }

    constexpr static bool is_always_invalid(uint8_t byte) {
        return (byte >= 0xC0 && byte <= 0xC1) || (byte >= 0xF5);
    }

    constexpr static bool is_valid_byte_offset(Span<char const> data, size_t index) {
        // The first index is always valid.
        if (index == 0) {
            return true;
        }

        // The last index is always valid, anything after is not.
        if (index >= data.size()) {
            return index == data.size();
        }

        // If the underlying byte delimits the first byte of a UTF-8 byte sequence, it is a boundary.
        char underlying_byte = data[index];
        if (Utf8::is_start_of_byte_sequence(underlying_byte)) {
            return true;
        }

        // If the underlying byte is invalid in any position, it will always be interpreted as a single U+FFFD code point.
        if (Utf8::is_always_invalid(underlying_byte)) {
            return true;
        }

        // Otherwise, the byte is the continuation of a multi-byte sequence.
        // However, if the underlying UTF-8 string is invalid, this byte may denote a new code point.

        // Check if this byte is valid as the second byte of multi-byte sequence.
        if (index > 0) {
            char first_byte = data[index - 1];
            if (Utf8::is_start_of_multi_byte_sequence(first_byte)) {
                return !Utf8::is_valid_second_byte(first_byte, underlying_byte);
            }
        }

        // Check if this byte is valid as the third byte of multi-byte sequence.
        if (index > 1) {
            char first_byte = data[index - 2];
            char second_byte = data[index - 1];
            if (Utf8::is_start_of_three_byte_sequence(first_byte) || Utf8::is_start_of_four_byte_sequence(first_byte)) {
                return !Utf8::is_valid_second_byte(first_byte, second_byte) || !Utf8::is_valid_third_byte(first_byte, underlying_byte);
            }
        }

        // Check if this byte is valid as the fourth byte of multi-byte sequence.
        if (index > 2) {
            char first_byte = data[index - 3];
            char second_byte = data[index - 2];
            char third_byte = data[index - 1];
            if (Utf8::is_start_of_four_byte_sequence(first_byte)) {
                return !Utf8::is_valid_second_byte(first_byte, second_byte) || !Utf8::is_valid_third_byte(first_byte, third_byte) ||
                       !Utf8::is_valid_fourth_byte(first_byte, underlying_byte);
            }
        }

        // This byte delimits an invalid UTF-8 byte sequence, and so is a boundary.
        return true;
    }

    // This function returns the code point value corresponding to the first byte sequence in data, and None {} if this byte sequence is
    // invalid UTF-8.
    constexpr Option<char32_t> code_point_value(Span<char const> data) {
        if (data.empty()) {
            return None {};
        }

        char first_byte = data[0];
        if (is_always_invalid(first_byte)) {
            return None {};
        }

        if (is_start_of_one_byte_sequence(first_byte)) {
            return first_byte;
        }

        if (is_start_of_two_byte_sequence(first_byte)) {
            if (data.size() < 2) {
                return None {};
            }

            char second_byte = data[1];
            if (!is_valid_second_byte(first_byte, second_byte)) {
                return None {};
            }

            return ((first_byte & 0b00011111) << 6) | (second_byte & 0b00111111);
        }

        if (is_start_of_three_byte_sequence(first_byte)) {
            if (data.size() < 3) {
                return None {};
            }

            char second_byte = data[1];
            char third_byte = data[2];
            if (!is_valid_second_byte(first_byte, second_byte) || !is_valid_third_byte(first_byte, third_byte)) {
                return None {};
            }

            return ((first_byte & 0b00001111) << 12) | ((second_byte & 0b00111111) << 6) | (third_byte & 0b00111111);
        }

        if (is_start_of_four_byte_sequence(first_byte)) {
            if (data.size() < 4) {
                return None {};
            }

            char second_byte = data[1];
            char third_byte = data[2];
            char fourth_byte = data[3];
            if (!is_valid_second_byte(first_byte, second_byte) || !is_valid_third_byte(first_byte, third_byte) ||
                !is_valid_fourth_byte(first_byte, fourth_byte)) {
                return None {};
            }

            return ((first_byte & 0b00000111) << 18) | ((second_byte & 0b00111111) << 12) | ((third_byte & 0b00111111) << 6) |
                   (fourth_byte & 0b00111111);
        }

        return None {};
    }
}

class Utf8Iterator {
public:
    using ValueType = char32_t;

    constexpr ValueType operator*() const { return current_code_point().value_or(Utf8::replacement_character); }

    constexpr Option<ValueType> current_code_point() const { return Utf8::code_point_value(m_data.subspan(m_index)); }

    constexpr size_t current_code_unit_offset() const { return m_index; }

    constexpr Utf8Iterator& operator++() {
        do {
            ++m_index;
        } while (!Utf8::is_valid_byte_offset(m_data, m_index));
        return *this;
    }

    constexpr Utf8Iterator& operator--() {
        do {
            --m_index;
        } while (!Utf8::is_valid_byte_offset(m_data, m_index));
        return *this;
    }

    constexpr Utf8Iterator operator++(int) {
        auto result = Utf8Iterator(*this);
        ++*this;
        return result;
    }

    constexpr Utf8Iterator operator--(int) {
        auto result = Utf8Iterator(*this);
        --*this;
        return result;
    }

    constexpr bool operator==(const Utf8Iterator& other) const { return this->m_index == other.m_index; }
    constexpr std::strong_ordering operator<=>(const Utf8Iterator& other) const { return this->m_index <=> other.m_index; }

private:
    constexpr Utf8Iterator(Span<char const> data, size_t index) : m_data(data), m_index(index) {}

    friend class Utf8Encoding;

    Span<char const> m_data;
    size_t m_index { 0 };
};

struct Utf8Encoding {
    using CodeUnit = char;
    using CodePoint = char32_t;
    using Iterator = Utf8Iterator;

    constexpr static auto code_point_iterators(Span<char const> data) {
        return iterator_container(Utf8Iterator(data, 0), Utf8Iterator(data, data.size()));
    }

    constexpr static bool is_valid(Span<char const> data) {
        auto [start, end] = code_point_iterators(data);
        for (auto it = start; it != end; ++it) {
            if (!it.current_code_point().has_value()) {
                return false;
            }
        }
        return true;
    }

    constexpr static bool is_valid_byte_offset(Span<char const> data, size_t index) { return Utf8::is_valid_byte_offset(data, index); }

    constexpr static Option<Iterator> iterator_at_offset(Span<char const> data, size_t offset) {
        if (!is_valid_byte_offset(data, offset)) {
            return None {};
        }
        return Utf8Iterator(data, offset);
    }
};
}
