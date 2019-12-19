#include <assert.h>
#include <stdio.h>

#include "sh_lexer.h"

ShLexer::~ShLexer() {}

bool ShLexer::lex() {
    bool in_d_quotes = false;
    bool in_s_quotes = false;
    bool prev_was_backslash = false;
    bool prev_was_dollar = false;

    auto find_end_of_word_expansion = [this](size_t start, bool __prev_was_dollar) -> size_t {
        bool in_b_quotes = false;
        bool in_s_quotes = false;
        bool in_d_quotes = false;
        bool prev_was_backslash = false;
        bool prev_was_dollar = __prev_was_dollar;

        enum class ParamExpansionType { Brace, Paren, DoubleParen };
        Stack<ParamExpansionType> param_stack;

        do {
            char current = this->m_input_stream[start];
            switch (current) {
                case '\\':
                    prev_was_backslash = !prev_was_backslash;
                    prev_was_dollar = false;
                    continue;
                case '$':
                    if (!prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes && !prev_was_dollar) {
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
                    if (prev_was_dollar && !prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes) {
                        param_stack.push(ParamExpansionType::Brace);
                    }
                    break;
                case '(':
                    if (!prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes) {
                        if (start + 1 < this->m_input_length && this->m_input_stream[start + 1] == '(') {
                            start++;
                            param_stack.push(ParamExpansionType::DoubleParen);
                        } else {
                            param_stack.push(ParamExpansionType::Paren);
                        }
                    }
                    break;
                case '}':
                    if (!prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes) {
                        if (param_stack.pop() != ParamExpansionType::Brace) {
                            return 0;
                        }
                    }
                    break;
                case ')':
                    if (!prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes) {
                        if (start + 1 < this->m_input_length && this->m_input_stream[start + 1] == ')') {
                            start++;
                            if (param_stack.pop() != ParamExpansionType::DoubleParen) {
                                return 0;
                            }
                        } else {
                            if (param_stack.pop() != ParamExpansionType::Paren) {
                                return 0;
                            }
                        }
                    }
                    break;
                default:
                    break;
            }

            prev_was_backslash = false;
            prev_was_dollar = false;
        } while (++start < this->m_input_length && (in_d_quotes || in_b_quotes || in_s_quotes || !param_stack.empty()));

        return !in_d_quotes && !in_b_quotes && !in_s_quotes && param_stack.empty() ? start - 1 : 0;
    };

    for (;;) {
        switch (peek()) {
            case EOF:
                commit_token(ShTokenType::WORD);
                return !in_d_quotes && !in_s_quotes;
            case '\\':
                prev_was_backslash = !prev_was_backslash;
                if (!m_current_token_start) {
                    begin_token();
                }
                consume();
                prev_was_dollar = false;
                continue;
            case '$':
                if (!prev_was_backslash && !prev_was_dollar) {
                    prev_was_dollar = true;
                    prev_was_backslash = false;
                    if (!m_current_token_start) {
                        begin_token();
                    }
                    consume();
                    continue;
                }
                goto process_regular_character;
            case '\t':
            case ' ':
                if (!prev_was_backslash && !in_d_quotes && !in_s_quotes) {
                    commit_token(ShTokenType::WORD);
                    consume();
                    break;
                }
                goto process_regular_character;
            case '\n':
                if (!prev_was_backslash && !in_d_quotes && !in_s_quotes) {
                    commit_token(ShTokenType::WORD);
                    begin_token();
                    consume();
                    commit_token(ShTokenType::NEWLINE);
                    break;
                }
                goto process_regular_character;
            case '\'':
                if (!prev_was_backslash && !in_d_quotes) {
                    in_s_quotes = !in_s_quotes;
                }
                goto process_regular_character;
            case '\"':
                if (!prev_was_backslash && !in_s_quotes) {
                    in_d_quotes = !in_d_quotes;
                }
                goto process_regular_character;
            case '#':
                if (!m_current_token_start && !prev_was_backslash && !in_s_quotes && !in_d_quotes) {
                    while (peek() != '\n' && peek() != EOF) {
                        consume();
                    }
                    break;
                }
                goto process_regular_character;
            case '|':
                if (!prev_was_backslash && !in_s_quotes && !in_d_quotes) {
                    commit_token(ShTokenType::WORD);
                    begin_token();
                    consume();
                    if (peek() == '|') {
                        consume();
                        commit_token(ShTokenType::OR_IF);
                    } else {
                        commit_token(ShTokenType::Pipe);
                    }
                    break;
                }
                goto process_regular_character;
            case '&':
                if (!prev_was_backslash && !in_s_quotes && !in_d_quotes) {
                    commit_token(ShTokenType::WORD);
                    begin_token();
                    consume();
                    if (peek() == '&') {
                        consume();
                        commit_token(ShTokenType::AND_IF);
                    } else {
                        commit_token(ShTokenType::Ampersand);
                    }
                    break;
                }
                goto process_regular_character;
            case ';':
                if (!prev_was_backslash && !in_s_quotes && !in_d_quotes) {
                    commit_token(ShTokenType::WORD);
                    begin_token();
                    consume();
                    if (peek() == ';') {
                        consume();
                        commit_token(ShTokenType::DSEMI);
                    } else {
                        commit_token(ShTokenType::Semicolon);
                    }
                    break;
                }
                goto process_regular_character;
            case '>':
                if (!prev_was_backslash && !in_s_quotes && !in_d_quotes) {
                    commit_token(ShTokenType::IO_NUMBER);
                    begin_token();
                    consume();
                    if (peek() == '>') {
                        consume();
                        commit_token(ShTokenType::DGREAT);
                    } else if (peek() == '&') {
                        consume();
                        commit_token(ShTokenType::GREATAND);
                    } else if (peek() == '|') {
                        consume();
                        commit_token(ShTokenType::CLOBBER);
                    } else {
                        commit_token(ShTokenType::GreaterThan);
                    }
                    break;
                }
                goto process_regular_character;
            case '<':
                if (!prev_was_backslash && !in_s_quotes && !in_d_quotes) {
                    commit_token(ShTokenType::IO_NUMBER);
                    begin_token();
                    consume();
                    if (peek() == '<') {
                        consume();
                        if (peek() == '-') {
                            consume();
                            commit_token(ShTokenType::DLESSDASH);
                        } else {
                            commit_token(ShTokenType::DLESS);
                        }
                    } else if (peek() == '>') {
                        consume();
                        commit_token(ShTokenType::LESSGREAT);
                    } else if (peek() == '&') {
                        consume();
                        commit_token(ShTokenType::LESSAND);
                    } else {
                        commit_token(ShTokenType::GreaterThan);
                    }
                    break;
                }
                goto process_regular_character;
            case '`':
                if (!prev_was_backslash && !in_s_quotes && !in_d_quotes) {
                    if (!m_current_token_start) {
                        begin_token();
                    }

                    size_t end_of_expansion = find_end_of_word_expansion(m_position, false);
                    if (end_of_expansion == 0) {
                        return false;
                    }

                    while (m_position <= end_of_expansion) {
                        consume();
                    }
                    break;
                }
                goto process_regular_character;
            case '{':
                if (!prev_was_backslash && !in_s_quotes && !in_d_quotes && prev_was_dollar) {
                    size_t end_of_expansion = find_end_of_word_expansion(m_position, true);
                    if (end_of_expansion == 0) {
                        return false;
                    }

                    while (m_position <= end_of_expansion) {
                        consume();
                    }
                    break;
                }
                goto process_regular_character;
            case '(':
                if (!prev_was_backslash && !in_s_quotes && !in_d_quotes && !prev_was_dollar) {
                    commit_token(ShTokenType::NAME);
                    begin_token();
                    consume();
                    commit_token(ShTokenType::LeftParenthesis);
                    break;
                }

                if (!prev_was_backslash && !in_s_quotes && !in_d_quotes && prev_was_dollar) {
                    size_t end_of_expansion = find_end_of_word_expansion(m_position, true);
                    if (end_of_expansion == 0) {
                        return false;
                    }

                    while (m_position <= end_of_expansion) {
                        consume();
                    }
                    break;
                }
                goto process_regular_character;
            case ')':
                if (!prev_was_backslash && !in_s_quotes && !in_d_quotes) {
                    commit_token(ShTokenType::WORD);
                    begin_token();
                    consume();
                    commit_token(ShTokenType::RightParenthesis);
                    break;
                }
                goto process_regular_character;
            process_regular_character:
            default:
                if (!m_current_token_start) {
                    begin_token();
                }
                consume();
                break;
        }

        prev_was_backslash = false;
        prev_was_dollar = false;
    }

    assert(false);
    return false;
}