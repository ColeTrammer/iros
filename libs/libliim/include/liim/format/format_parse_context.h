#pragma once

#include <liim/maybe.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <stdlib.h>

namespace LIIM::Format {
class FormatParseContext {
public:
    Maybe<size_t> parse_arg_index(const StringView& view) {
        m_view = view;

        auto index = parse_index();
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

    Maybe<size_t> parse_index() {
        auto string = ""s;
        while (!empty() && isdigit(peek())) {
            string += String(take());
        }
        if (string.empty()) {
            return {};
        }
        return { static_cast<size_t>(atoi(string.string())) };
    }

    bool empty() const { return m_view.empty(); }
    char peek() const { return m_view.first(); }
    char take() {
        auto ret = m_view.first();
        m_view = m_view.substring(1);
        return ret;
    }

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
