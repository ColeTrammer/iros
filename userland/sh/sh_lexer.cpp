#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#ifndef USERLAND_NATIVE
#include <wordexp.h>
#else
#include "../../libs/libc/include/wordexp.h"
#endif /* USERLAND_NATIVE */

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
                    if (!in_s_quotes) {
                        prev_was_backslash = !prev_was_backslash;
                        prev_was_dollar = false;
                        continue;
                    }
                    break;
                case '$':
                    if (!prev_was_backslash && !in_d_quotes && !in_s_quotes && !in_b_quotes && !prev_was_dollar) {
                        prev_was_dollar = true;
                        continue;
                    }
                    break;
                case '\'':
                    in_s_quotes = !in_d_quotes && !in_b_quotes ? !in_s_quotes : in_s_quotes;
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
                if (!in_s_quotes) {
                    prev_was_backslash = !prev_was_backslash;
                    if (!m_current_token_start) {
                        begin_token();
                    }
                    consume();
                    prev_was_dollar = false;
                    continue;
                }
                goto process_regular_character;
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

                    // For Here documents
                    resume_position_if_needed();
                    break;
                }
                goto process_regular_character;
            case '\'':
                if (!in_d_quotes) {
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
                        bool strip_leading_tabs = false;
                        if (peek() == '-') {
                            consume();
                            commit_token(ShTokenType::DLESSDASH);
                            strip_leading_tabs = true;
                        } else if (peek() == '<') {
                            consume();
                            commit_token(ShTokenType::TLESS);
                            break;
                        } else {
                            commit_token(ShTokenType::DLESS);
                        }

                        while (isspace(peek())) {
                            consume();
                        }

                        size_t word_start = m_position;
                        for (;;) {
                            switch (peek()) {
                                case ' ':
                                case '\t':
                                case '\n':
                                case '|':
                                case ';':
                                case '<':
                                case '>':
                                case '(':
                                case ')':
                                case '&':
                                    break;
                                case EOF:
                                    // Obviously need more characters in this case
                                    return false;
                                default:
                                    consume();
                                    continue;
                            }
                            break;
                        }

                        size_t pos_save = m_position;
                        size_t cont_row_save = m_current_row;
                        size_t cont_col_save = m_current_col;

                        StringView here_end(m_input_stream + word_start, m_input_stream + m_position - 1);
                        while (peek() != '\n') {
                            consume();
                            if (peek() == EOF) {
                                return false;
                            }
                        }

                        consume(); // Trailing `\n'

                        // Handle multipe here documents
                        resume_position_if_needed();

                        char* here_end_unescaped = strdup(String(here_end).string());
                        int ret = we_unescape(&here_end_unescaped);
                        assert(ret != WRDE_NOSPACE);

                        StringView new_here_end(const_cast<const char*>(here_end_unescaped));
                        ShValue::IoRedirect::HereDocumentQuoted here_document_quoted = new_here_end == here_end
                                                                                           ? ShValue::IoRedirect::HereDocumentQuoted::No
                                                                                           : ShValue::IoRedirect::HereDocumentQuoted::Yes;

                        size_t row_save = m_current_row;
                        size_t col_save = m_current_col;
                        size_t here_document_start = m_position;
                        size_t line_start = m_position;
                        for (;;) {
                            if (strip_leading_tabs) {
                                while (peek() == '\t') {
                                    consume();
                                }
                            }

                            if (peek() == EOF && new_here_end.size() != 0) {
                                free(here_end_unescaped);
                                return false;
                            }

                            line_start = m_position;
                            while (peek() != EOF && peek() != '\n') {
                                consume();
                            }

                            StringView entire_line(m_input_stream + line_start, m_input_stream + m_position - 1);

                            if (m_position - line_start <= 1) {
                                if (new_here_end.size() == 0) {
                                    goto heredoc_line_matches;
                                }

                                continue; // Don't compare empty lines
                            }

                            if (entire_line == new_here_end) {
                            heredoc_line_matches:
                                free(here_end_unescaped);
                                consume();
                                break;
                            }

                            if (peek() == EOF) {
                                free(here_end_unescaped);
                                return false;
                            }

                            consume();
                        }

                        StringView here_document(m_input_stream + here_document_start, m_input_stream + line_start - 1);
                        m_tokens.add({ ShTokenType::WORD, { here_document, row_save, col_save } });
                        m_tokens.last().value().create_io_redirect(STDIN_FILENO, ShValue::IoRedirect::Type::HereDocument, here_document,
                                                                   here_document_quoted);
                        set_resume_position();

                        m_position = pos_save;
                        m_current_row = cont_row_save;
                        m_current_row = cont_col_save;
                    } else if (peek() == '>') {
                        consume();
                        commit_token(ShTokenType::LESSGREAT);
                    } else if (peek() == '&') {
                        consume();
                        commit_token(ShTokenType::LESSAND);
                    } else {
                        commit_token(ShTokenType::LessThan);
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