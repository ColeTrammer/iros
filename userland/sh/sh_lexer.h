#pragma once

#include <ctype.h>
#include <liim/string_view.h>
#include <liim/vector.h>
#include <stddef.h>
#include <stdio.h>

#include "sh_parser.h"

class ShLexer {
public:
    ShLexer(char* input_stream, size_t length) : m_input_stream(input_stream), m_input_length(length) {}
    ~ShLexer();

    bool lex();

    const Vector<ShParser::Token>& tokens() const { return m_tokens; }

private:
    int peek() const {
        if (m_position >= m_input_length) {
            return EOF;
        }
        return m_input_stream[m_position];
    }

    void consume() {
        if (peek() == '\n') {
            m_current_row++;
            m_current_col = -1;
        }

        m_position++;
        m_current_col++;
    }

    void begin_token() {
        m_current_token_start = m_input_stream + m_position;
        m_current_token_row = m_current_row;
        m_current_token_col = m_current_col;
    }

    void commit_token(ShTokenType type) {
        if (!m_current_token_start) {
            if (type == ShTokenType::NAME && m_tokens.last().type() == ShTokenType::WORD) {
                m_tokens.last().set_type(ShTokenType::NAME);
            }
            return;
        }

        StringView text = { m_current_token_start, m_input_stream + m_position - 1 };
        if (m_expecting_name) {
            m_expecting_name = false;

            if (type == ShTokenType::WORD) {
                type = ShTokenType::NAME;
            }
        }

        if (type == ShTokenType::IO_NUMBER) {
            for (int i = 0; i < text.end() - text.start(); i++) {
                if (!isdigit(text.start()[i])) {
                    type = ShTokenType::WORD;
                    break;
                }
            }
        } else if (type == ShTokenType::WORD) {
            if (text == "if") {
                type = ShTokenType::If;
            } else if (text == "then") {
                type = ShTokenType::Then;
            } else if (text == "else") {
                type = ShTokenType::Else;
            } else if (text == "elif") {
                type = ShTokenType::Elif;
            } else if (text == "fi") {
                type = ShTokenType::Fi;
            } else if (text == "do") {
                type = ShTokenType::Do;
            } else if (text == "done") {
                type = ShTokenType::Done;
            } else if (text == "case") {
                type = ShTokenType::Case;
            } else if (text == "esac") {
                type = ShTokenType::Esac;
            } else if (text == "while") {
                type = ShTokenType::While;
            } else if (text == "until") {
                type = ShTokenType::Until;
            } else if (text == "for") {
                type = ShTokenType::For;
                m_expecting_name = true;
            } else if (text == "{") {
                type = ShTokenType::Lbrace;
            } else if (text == "}") {
                type = ShTokenType::Rbrace;
            } else if (text == "!") {
                type = ShTokenType::Bang;
            } else if (text == "in") {
                type = ShTokenType::In;
            }

            bool in_s_quotes = false;
            bool in_d_quotes = false;
            bool in_b_quotes = false;
            bool prev_was_backslash = false;
            bool prev_was_dollar = false;
            bool found_equal = false;
            int param_expansion_count = 0;
            for (int i = 0; !found_equal && i < text.size(); i++) {
                char current = text.start()[i];
                char next = i + 1 < text.size() ? text.start()[i + 1] : '\0';
                switch (current) {
                    case '\\':
                        prev_was_backslash = !prev_was_backslash;
                        prev_was_dollar = false;
                        continue;
                    case '$':
                        if (!prev_was_dollar && !prev_was_backslash && !in_d_quotes && !in_b_quotes && !in_s_quotes) {
                            prev_was_dollar = true;
                            continue;
                        }
                        break;
                    case '\'':
                        in_s_quotes = !prev_was_backslash && !in_d_quotes && !in_b_quotes ? !in_s_quotes : in_s_quotes;
                        break;
                    case '"':
                        in_d_quotes = !prev_was_backslash && !in_s_quotes && !in_b_quotes ? !in_d_quotes : in_d_quotes;
                        break;
                    case '`':
                        in_b_quotes = !prev_was_backslash && !in_d_quotes && !in_s_quotes ? !in_b_quotes : in_b_quotes;
                        break;
                    case '{':
                        if (prev_was_dollar && !prev_was_backslash && !in_b_quotes && !in_s_quotes && !in_d_quotes) {
                            param_expansion_count++;
                        }
                        break;
                    case '(':
                        if (!prev_was_backslash && !in_b_quotes && !in_s_quotes && !in_d_quotes) {
                            param_expansion_count++;
                        }
                        break;
                    case '}':
                    case ')':
                        if (!prev_was_backslash && !in_b_quotes && !in_s_quotes && !in_d_quotes) {
                            param_expansion_count--;
                        }
                        break;
                    case '=':
                        if (!prev_was_backslash && !in_b_quotes && !in_s_quotes && !in_d_quotes && param_expansion_count == 0) {
                            found_equal = true;
                        }
                        break;
                    default:
                        break;
                }

                prev_was_dollar = false;
                prev_was_backslash = false;
            }

            if (found_equal && !(m_tokens.last().type() != ShTokenType::WORD && text.start()[0] == '=')) {
                type = ShTokenType::ASSIGNMENT_WORD;
            }
        }

        m_tokens.add({ type, { text, m_current_token_row, m_current_token_col } });
        m_current_token_start = nullptr;
        m_current_token_row = 0;
        m_current_token_col = 0;
    }

    char* m_input_stream { nullptr };
    size_t m_input_length { 0 };
    size_t m_position { 0 };
    size_t m_current_row { 0 };
    size_t m_current_col { 0 };
    size_t m_current_token_row { 0 };
    size_t m_current_token_col { 0 };
    char* m_current_token_start { nullptr };
    bool m_expecting_name { false };
    Vector<ShParser::Token> m_tokens;
};