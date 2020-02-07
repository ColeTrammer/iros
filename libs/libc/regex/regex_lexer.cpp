#include <ctype.h>
#include <regex.h>

#include "regex_lexer.h"

RegexLexer::~RegexLexer() {}

bool RegexLexer::lex() {
    bool prev_was_backslash = false;
    while (peek() != '\0') {
        switch (peek()) {
            case '\0':
                break;
            case '\\':
                consume();
                if (prev_was_backslash) {
                    commit_token(RegexTokenType::QuotedCharacter);
                    break;
                }
                prev_was_backslash = true;
                continue;
            case '[':
                consume();
                if (prev_was_backslash) {
                    commit_token(RegexTokenType::QuotedCharacter);
                    break;
                }
                commit_token(RegexTokenType::LeftSquareBracket);
                if (peek() == '^') {
                    consume();
                    commit_token(RegexTokenType::Carrot);
                }
                if (peek() == ']') {
                    consume();
                    commit_token(RegexTokenType::CollateSingleElement);
                } else if (peek() == '-') {
                    consume();
                    commit_token(RegexTokenType::CollateSingleElement);
                }
                while (peek() != '\0' && peek() != ']') {
                    switch (peek()) {
                        case '-':
                            consume();
                            if (peek() == ']' || m_tokens.last().type() == RegexTokenType::Minus) {
                                commit_token(RegexTokenType::CollateSingleElement);
                            } else {
                                commit_token(RegexTokenType::Minus);
                            }
                            break;
                        case '[':
                            consume();
                            switch (peek()) {
                                case '.':
                                    consume();
                                    commit_token(RegexTokenType::LeftSquareBracketPeriod);
                                    while (peek() && peek(2) != ".]") {
                                        consume();
                                    }
                                    if (!peek()) {
                                        m_error_code = REG_EBRACK;
                                        return false;
                                    }
                                    if (m_position - m_token_start > 1) {
                                        commit_token(RegexTokenType::CollateMultipleElements);
                                    } else if (prev() == '-' || prev() == ']') {
                                        commit_token(RegexTokenType::MetaCharacter);
                                    } else {
                                        commit_token(RegexTokenType::CollateSingleElement);
                                    }
                                    consume();
                                    consume();
                                    commit_token(RegexTokenType::PeriodRightSquareBracket);
                                    break;
                                case '=':
                                    consume();
                                    commit_token(RegexTokenType::LeftSquareBracketEqual);
                                    while (peek() && peek(2) != "=]") {
                                        consume();
                                    }
                                    if (!peek()) {
                                        m_error_code = REG_EBRACK;
                                        return false;
                                    }
                                    if (m_position - m_token_start > 1) {
                                        commit_token(RegexTokenType::CollateMultipleElements);
                                    } else {
                                        commit_token(RegexTokenType::CollateSingleElement);
                                    }
                                    consume();
                                    consume();
                                    commit_token(RegexTokenType::EqualRightSquareBracket);
                                    break;
                                case ':':
                                    consume();
                                    commit_token(RegexTokenType::LeftSquareBracketColon);
                                    while (peek() && peek(2) != ":]") {
                                        consume();
                                    }
                                    if (!peek()) {
                                        m_error_code = REG_EBRACK;
                                        return false;
                                    }
                                    commit_token(RegexTokenType::ClassName);
                                    consume();
                                    consume();
                                    commit_token(RegexTokenType::ColonRightSquareBracket);
                                    break;
                                default:
                                    commit_token(RegexTokenType::CollateSingleElement);
                                    break;
                            }
                            break;
                        default:
                            consume();
                            commit_token(RegexTokenType::CollateSingleElement);
                            break;
                    }
                }
                if (peek() == '\0') {
                    // Mis matched square bracket
                    m_error_code = REG_EBRACK;
                    return false;
                }
                consume();
                commit_token(RegexTokenType::RightSquareBracket);
                break;
            case '.':
                consume();
                if (prev_was_backslash) {
                    commit_token(RegexTokenType::QuotedCharacter);
                } else {
                    commit_token(RegexTokenType::Period);
                }
                break;
            case '*':
                consume();
                if (prev_was_backslash) {
                    commit_token(RegexTokenType::QuotedCharacter);
                } else if (m_tokens.empty() || m_tokens.last().type() == RegexTokenType::LeftAnchor ||
                           m_tokens.last().type() == RegexTokenType::LeftParenthesis || m_tokens.last().type() == RegexTokenType::Pipe) {
                    commit_token(RegexTokenType::OrdinaryCharacter);
                } else {
                    commit_token(RegexTokenType::Asterisk);
                }
                break;
            case '^':
                consume();
                if (prev_was_backslash) {
                    commit_token(RegexTokenType::QuotedCharacter);
                } else if ((m_flags & REG_EXTENDED) || m_position == 0) {
                    commit_token(RegexTokenType::Carrot);
                } else {
                    commit_token(RegexTokenType::OrdinaryCharacter);
                }
                break;
            case '$':
                consume();
                if (prev_was_backslash) {
                    commit_token(RegexTokenType::QuotedCharacter);
                } else if ((m_flags & REG_EXTENDED) || peek() == '\0') {
                    commit_token(RegexTokenType::Dollar);
                } else {
                    commit_token(RegexTokenType::OrdinaryCharacter);
                }
                break;
            case ',':
                consume();
                if (m_tokens.last().type() == RegexTokenType::DuplicateCount) {
                    commit_token(RegexTokenType::Comma);
                } else {
                    commit_token(RegexTokenType::OrdinaryCharacter);
                }
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                consume();
                if (prev_was_backslash) {
                    if (prev() == '0' || (m_flags & REG_NOSUB)) {
                        m_error_code = REG_ESUBREG;
                        return false;
                    } else {
                        commit_token(RegexTokenType::BackReference);
                    }
                } else if (m_tokens.last().type() == RegexTokenType::LeftCurlyBrace || m_tokens.last().type() == RegexTokenType::Comma) {
                    while (isdigit(peek())) {
                        consume();
                    }
                    commit_token(RegexTokenType::DuplicateCount);
                } else {
                    commit_token(RegexTokenType::OrdinaryCharacter);
                }
                break;
            case '(':
                consume();
                if (prev_was_backslash ^ (m_flags & REG_EXTENDED)) {
                    commit_token(RegexTokenType::LeftParenthesis);
                    m_group_incidices.put(m_position - 2, ++m_group_count);
                } else {
                    commit_token(m_flags & REG_EXTENDED ? RegexTokenType::QuotedCharacter : RegexTokenType::OrdinaryCharacter);
                }
                break;
            case ')':
                consume();
                if (prev_was_backslash ^ (m_flags & REG_EXTENDED)) {
                    commit_token(RegexTokenType::RightParenthesis);
                } else {
                    commit_token(m_flags & REG_EXTENDED ? RegexTokenType::QuotedCharacter : RegexTokenType::OrdinaryCharacter);
                }
                break;
            case '{':
                consume();
                if (prev_was_backslash ^ (m_flags & REG_EXTENDED)) {
                    commit_token(RegexTokenType::RightCurlyBrace);
                } else {
                    commit_token(m_flags & REG_EXTENDED ? RegexTokenType::QuotedCharacter : RegexTokenType::OrdinaryCharacter);
                }
                break;
            case '}':
                consume();
                if (prev_was_backslash ^ (m_flags & REG_EXTENDED)) {
                    commit_token(RegexTokenType::LeftCurlyBrace);
                } else {
                    commit_token(m_flags & REG_EXTENDED ? RegexTokenType::QuotedCharacter : RegexTokenType::OrdinaryCharacter);
                }
                break;
            case '|':
                consume();
                if (!prev_was_backslash && (m_flags & REG_EXTENDED)) {
                    commit_token(RegexTokenType::Pipe);
                } else {
                    commit_token(m_flags & REG_EXTENDED ? RegexTokenType::QuotedCharacter : RegexTokenType::OrdinaryCharacter);
                }
                break;
            case '+':
                consume();
                if (prev_was_backslash ^ (m_flags & REG_EXTENDED)) {
                    commit_token(RegexTokenType::Plus);
                } else {
                    commit_token(m_flags & REG_EXTENDED ? RegexTokenType::QuotedCharacter : RegexTokenType::OrdinaryCharacter);
                }
                break;
            case '?':
                consume();
                if (prev_was_backslash ^ (m_flags & REG_EXTENDED)) {
                    commit_token(RegexTokenType::QuestionMark);
                } else {
                    commit_token(m_flags & REG_EXTENDED ? RegexTokenType::QuotedCharacter : RegexTokenType::OrdinaryCharacter);
                }
                break;
            default:
                consume();
                if (prev_was_backslash) {
                    commit_token(RegexTokenType::QuotedCharacter);
                } else {
                    commit_token(RegexTokenType::OrdinaryCharacter);
                }
                break;
        }
        prev_was_backslash = false;
    }

    if (prev_was_backslash) {
        m_error_code = REG_EESCAPE;
        return false;
    }

    return true;
}