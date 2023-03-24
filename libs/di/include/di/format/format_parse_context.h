#pragma once

#include <di/container/string/encoding.h>
#include <di/container/string/string_view_impl.h>
#include <di/parser/prelude.h>
#include <di/util/compile_time_fail.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/variant/prelude.h>

namespace di::format {
template<concepts::Encoding Enc>
class FormatParseContext {
private:
    using Iter = meta::EncodingIterator<Enc>;
    using View = container::string::StringViewImpl<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;

    enum class IndexingMode {
        Unknown,
        Manual,
        Automatic,
    };

public:
    constexpr explicit FormatParseContext(View view, size_t arg_count) : m_view(view), m_arg_count(arg_count) {}

    struct Argument {
        View format_string;
        size_t index;
    };

    using Value = Result<Variant<View, Argument>>;

    struct Iterator : public container::IteratorBase<Iterator, InputIteratorTag, Value, ssize_t> {
    private:
        constexpr explicit Iterator(View data, FormatParseContext& parent)
            : m_data(data), m_position(data.begin()), m_parent(util::addressof(parent)) {
            advance_one();
        }

        friend class FormatParseContext;

    public:
        using Encoding = Enc;

        Iterator() = default;

        constexpr Value&& operator*() const { return util::move(m_parent->m_current_value); }

        constexpr void advance_one() {
            if (m_position == m_data.end()) {
                m_at_end = true;
                return;
            }

            auto start = m_position;

            // Try to parse a format arguments.
            if (*m_position == CodePoint('{')) {
                ++m_position;

                // Unclosed '{'.
                if (m_position == m_data.end()) {
                    return set_error(BasicError::Invalid);
                }

                // Escaped '{'.
                if (*m_position == CodePoint('{')) {
                    set_value(m_data.substr(start, m_position));
                    ++m_position;
                    return;
                }

                // Find the closing '}', allowing inner nesting of '{' and'}' pairs.
                size_t left_brace_count = 1;
                for (; left_brace_count && m_position != m_data.end(); ++m_position) {
                    if (*m_position == CodePoint('{')) {
                        ++left_brace_count;
                    } else if (*m_position == CodePoint('}')) {
                        --left_brace_count;
                    }
                }

                if (left_brace_count > 0) {
                    return set_error(BasicError::Invalid);
                }

                auto inside_view = m_data.substr(container::next(start), container::prev(m_position));

                using namespace integral_set_literals;

                auto inside_view_context = parser::StringViewParserContext<Enc>(inside_view);
                size_t arg_index = 0;

                // Handle manual format string indexing: '{0:...}' and '{15:...}'.
                auto digit_parser = parser::match_zero_or_more('0'_m - '9'_m);
                auto digit_result = *digit_parser.parse(inside_view_context);
                if (!digit_result.empty()) {
                    auto index = parser::parse<size_t>(digit_result);
                    if (!index) {
                        return set_error(util::move(index).error());
                    }
                    auto result = m_parent->check_arg_index(*index);
                    if (!result) {
                        return set_error(util::move(result).error());
                    }
                    inside_view.replace_begin(digit_result.end());
                    arg_index = *index;
                } else {
                    // Handle automatic index mode.
                    auto result = m_parent->next_arg_index();
                    if (!result) {
                        return set_error(util::move(result).error());
                    }
                    arg_index = *result;
                }

                // At this point, the inside view must either be empty or begin with a ':'.
                if (!inside_view.empty() && !inside_view.starts_with(CodePoint(':'))) {
                    return set_error(BasicError::Invalid);
                }

                // Skip over colon.
                if (!inside_view.empty()) {
                    inside_view = inside_view.substr(container::next(inside_view.begin()));
                }

                // Return an object containing the interior of the format string, as well as
                // the specified argument index.
                return set_value(Argument { inside_view, arg_index });
            }

            // A '}' must be escaped.
            if (*m_position == CodePoint('}')) {
                ++m_position;

                // Unclosed '}'.
                if (m_position == m_data.end() || *m_position != CodePoint('}')) {
                    return set_error(BasicError::Invalid);
                }

                set_value(m_data.substr(start, m_position));
                ++m_position;
                return;
            }

            // Literal string, advance until a '{', '}', or EOF is reached.
            do {
                ++m_position;
            } while (m_position != m_data.end() && *m_position != CodePoint('{') && *m_position != CodePoint('}'));
            set_value(m_data.substr(start, m_position));
        }

    private:
        constexpr friend bool operator==(Iterator const& a, container::DefaultSentinel) { return a.m_at_end; }

        constexpr void set_error(Error error) {
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wredundant-consteval-if"
#endif
            if consteval {
                util::compile_time_fail<FixedString { "Invalid format string." }>();
            }
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
            m_position = m_data.end();
            set_value(util::move(error));
        }

        constexpr void set_value(View view) { m_parent->m_current_value.emplace(in_place_index<0>, view); }
        constexpr void set_value(Argument argument) {
            m_parent->m_current_format_string = argument.format_string;
            m_parent->m_current_value.emplace(in_place_index<1>, argument);
        }
        constexpr void set_value(Error error) { m_parent->m_current_value = Unexpected(util::move(error)); }

        View m_data {};
        Iter m_position {};
        FormatParseContext* m_parent { nullptr };
        bool m_at_end { false };
    };

    constexpr auto begin() { return Iterator(m_view, *this); }
    constexpr auto end() const { return container::default_sentinel; }

    constexpr void set_current_format_string(View view) { m_current_format_string = view; }
    constexpr View current_format_string() const { return m_current_format_string; }

    constexpr Result<size_t> next_arg_index() {
        if (m_indexing_mode == IndexingMode::Manual) {
            return Unexpected(BasicError::Invalid);
        }
        m_indexing_mode = IndexingMode::Automatic;

        auto index = m_next_arg_index++;
        if (index >= m_arg_count) {
            return Unexpected(BasicError::Invalid);
        }
        return index;
    }

    constexpr Result<void> check_arg_index(size_t index) {
        if (m_indexing_mode == IndexingMode::Automatic) {
            return Unexpected(BasicError::Invalid);
        }
        m_indexing_mode = IndexingMode::Manual;

        if (index >= m_arg_count) {
            return Unexpected(BasicError::Invalid);
        }
        return {};
    }

private:
    View m_view;
    IndexingMode m_indexing_mode { IndexingMode::Unknown };
    Value m_current_value;
    View m_current_format_string;
    size_t m_next_arg_index { 0 };
    size_t m_arg_count { 0 };
};
}
