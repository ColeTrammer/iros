#pragma once

#include <liim/character_type.h>
#include <liim/maybe.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <stdlib.h>

namespace LIIM::Format {
class FormatParseContext {
public:
    constexpr Maybe<size_t> parse_arg_index(const StringView& view) {
        m_view = view;

        auto index = parse_number();
        if (!index) {
            if (m_indexing_mode == IndexingMode::Manual) {
                // Error: cannot switch from manual to automatic indexing mode.
                return {};
            }
            m_indexing_mode = IndexingMode::Automatic;
            return { m_next_index++ };
        }

        if (m_indexing_mode == IndexingMode::Automatic) {
            // Error: cannot switch from automatic to manual indexing mode.
            return {};
        }
        m_indexing_mode = IndexingMode::Manual;
        return { *index };
    }

    constexpr bool parse_colon() {
        if (empty()) {
            return true;
        }
        if (*peek() != ':') {
            // Error: format specifier expected a ':'
            return false;
        }
        take();
        return true;
    }

    constexpr Maybe<size_t> parse_number() {
        size_t digit_count = 0;
        while (peek(digit_count) && is_digit(*peek(digit_count))) {
            digit_count++;
        }
        return ::parse_number(take(digit_count));
    }

    constexpr bool empty() const { return m_view.empty(); }
    constexpr Maybe<char> peek(size_t lookahead = 0) const {
        if (lookahead >= m_view.size()) {
            return {};
        }
        return m_view[lookahead];
    }
    constexpr StringView take(size_t count = 1) {
        auto ret = m_view.first(count);
        m_view = m_view.substring(count);
        return ret;
    }

    constexpr StringView view() const { return m_view; }

private:
    enum class IndexingMode {
        Undetermined,
        Manual,
        Automatic,
    };

    StringView m_view;
    size_t m_next_index { 0 };
    IndexingMode m_indexing_mode { IndexingMode::Undetermined };
};
}
