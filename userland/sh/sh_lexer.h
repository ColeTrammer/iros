#pragma once

#include <ctype.h>
#include <liim/stack.h>
#include <liim/string_view.h>
#include <liim/vector.h>
#include <parser/generic_lexer.h>
#include <stddef.h>
#include <stdio.h>

#include "generic_sh_parser.h"
#include "sh_token.h"
#include "sh_token_type.h"

class ShLexer final : public GenericLexer<ShTokenType, ShValue> {
public:
    using Token = GenericToken<ShTokenType, ShValue>;

    ShLexer(char* input_stream, size_t length)
        : GenericLexer<ShTokenType, ShValue>(), m_input_stream(input_stream), m_input_length(length) {}
    virtual ~ShLexer();

    bool lex();

    const Vector<Token>& tokens() const { return m_tokens; }

    virtual ShTokenType peek_next_token_type() const override {
        if (m_current_pos >= (size_t) m_tokens.size()) {
            return ShTokenType::End;
        }

        ShTokenType type = m_tokens[m_current_pos].type();

        if (type == ShTokenType::WORD) {
            const StringView& text = m_tokens[m_current_pos].value().text();

            if (m_parser && m_parser->is_valid_token_type_in_current_state_for_shift(ShTokenType::ASSIGNMENT_WORD) &&
                type == ShTokenType::WORD) {
                bool in_s_quotes = false;
                bool in_d_quotes = false;
                bool in_b_quotes = false;
                bool prev_was_backslash = false;
                bool prev_was_dollar = false;
                bool found_equal = false;
                int param_expansion_count = 0;
                for (int i = 0; !found_equal && i < text.size(); i++) {
                    char current = text.start()[i];
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

                if (found_equal && !((m_current_pos == 0 || would_be_first_word_of_command(m_current_pos)) && text.start()[0] == '=')) {
                    type = ShTokenType::ASSIGNMENT_WORD;
                }
            }

            if (type != ShTokenType::WORD) {
                const_cast<ShLexer&>(*this).m_tokens[m_current_pos].set_type(type);
            }
        }

        return type;
    }

    virtual const ShValue& peek_next_token_value() const override {
        assert(m_current_pos < (size_t) m_tokens.size());
        return m_tokens[m_current_pos].value();
    }

    virtual void advance() override { m_current_pos++; }

    void set_parser(GenericShParser<ShValue>& parser) { m_parser = &parser; }

private:
    static bool is_io_redirect(ShTokenType type) {
        return type == ShTokenType::LESSGREAT || type == ShTokenType::LessThan || type == ShTokenType::GreaterThan ||
               type == ShTokenType::GREATAND || type == ShTokenType::LESSGREAT || type == ShTokenType::DGREAT ||
               type == ShTokenType::DLESS || type == ShTokenType::DLESSDASH;
    }

    bool would_be_first_word_of_command(int start_index = -1) const {
        for (int i = start_index == -1 ? m_tokens.size() - 1 : start_index - 1; i >= 0; i--) {
            if (m_tokens[i].type() != ShTokenType::WORD) {
                return true;
            } else if (i > 0 && is_io_redirect(m_tokens[i - 1].type())) {
                i--;
                continue;
            } else {
                return false;
            }
        }

        return true;
    }

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
            if (type == ShTokenType::NAME && m_tokens.size() && m_tokens.last().type() == ShTokenType::WORD) {
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
            if (would_be_first_word_of_command() || m_allow_reserved_word_next) {
                m_allow_reserved_word_next = true;
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
                    m_allow_reserved_word_next = false;
                    type = ShTokenType::Case;
                } else if (text == "esac") {
                    type = ShTokenType::Esac;
                } else if (text == "while") {
                    type = ShTokenType::While;
                } else if (text == "until") {
                    type = ShTokenType::Until;
                } else if (text == "for") {
                    m_expecting_name = true;
                    m_allow_reserved_word_next = false;
                    type = ShTokenType::For;
                } else if (text == "{") {
                    type = ShTokenType::Lbrace;
                } else if (text == "}") {
                    type = ShTokenType::Rbrace;
                } else if (text == "!") {
                    type = ShTokenType::Bang;
                } else if (text == "in") {
                    m_allow_reserved_word_next = false;
                    type = ShTokenType::In;
                } else {
                    m_allow_reserved_word_next = false;
                }
            }

            if (m_tokens.size() >= 2 && m_tokens[m_tokens.size() - 2].type() == ShTokenType::Case && text == "in") {
                m_allow_reserved_word_next = false;
                type = ShTokenType::In;
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
    bool m_allow_reserved_word_next { false };
    size_t m_current_pos { 0 };
    GenericShParser<ShValue>* m_parser { nullptr };
    Vector<Token> m_tokens;
};