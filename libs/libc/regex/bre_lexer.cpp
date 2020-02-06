#include <ctype.h>
#include <regex.h>

#include "bre_lexer.h"

BRELexer::~BRELexer() {}

bool BRELexer::lex() {
    if (peek() == '^') {
        consume();
        commit_token(BasicTokenType::LeftAnchor);
    }

    bool prev_was_backslash = false;
    while (peek() != '\0') {

        switch (peek()) {
            case '\0':
                break;
            case '\\':
                consume();
                if (prev_was_backslash) {
                    commit_token(BasicTokenType::QuotedCharacter);
                    break;
                }
                prev_was_backslash = true;
                continue;
            case '[':
                consume();
                if (prev_was_backslash) {
                    commit_token(BasicTokenType::QuotedCharacter);
                    break;
                }
                commit_token(BasicTokenType::LeftSquareBracket);
                if (peek() == '^') {
                    consume();
                    commit_token(BasicTokenType::Carrot);
                }
                if (peek() == ']') {
                    consume();
                    commit_token(BasicTokenType::CollateSingleElement);
                } else if (peek() == '-') {
                    consume();
                    commit_token(BasicTokenType::CollateSingleElement);
                }
                while (peek() != '\0' && peek() != ']') {
                    switch (peek()) {
                        case '-':
                            consume();
                            if (peek() == ']' || m_tokens.last().type() == BasicTokenType::Minus) {
                                commit_token(BasicTokenType::CollateSingleElement);
                            } else {
                                commit_token(BasicTokenType::Minus);
                            }
                            break;
                        case '[':
                            consume();
                            switch (peek()) {
                                case '.':
                                    consume();
                                    commit_token(BasicTokenType::LeftSquareBracketPeriod);
                                    while (peek() && peek(2) != ".]") {
                                        consume();
                                    }
                                    if (!peek()) {
                                        m_error_code = REG_EBRACK;
                                        return false;
                                    }
                                    if (m_position - m_token_start > 1) {
                                        commit_token(BasicTokenType::CollateMultipleElements);
                                    } else if (prev() == '-' || prev() == ']') {
                                        commit_token(BasicTokenType::MetaCharacter);
                                    } else {
                                        commit_token(BasicTokenType::CollateSingleElement);
                                    }
                                    consume();
                                    consume();
                                    commit_token(BasicTokenType::PeriodRightSquareBracket);
                                    break;
                                case '=':
                                    consume();
                                    commit_token(BasicTokenType::LeftSquareBracketEqual);
                                    while (peek() && peek(2) != "=]") {
                                        consume();
                                    }
                                    if (!peek()) {
                                        m_error_code = REG_EBRACK;
                                        return false;
                                    }
                                    if (m_position - m_token_start > 1) {
                                        commit_token(BasicTokenType::CollateMultipleElements);
                                    } else {
                                        commit_token(BasicTokenType::CollateSingleElement);
                                    }
                                    consume();
                                    consume();
                                    commit_token(BasicTokenType::EqualRightSquareBracket);
                                    break;
                                case ':':
                                    consume();
                                    commit_token(BasicTokenType::LeftSquareBracketColon);
                                    while (peek() && peek(2) != ":]") {
                                        consume();
                                    }
                                    if (!peek()) {
                                        m_error_code = REG_EBRACK;
                                        return false;
                                    }
                                    commit_token(BasicTokenType::ClassName);
                                    consume();
                                    consume();
                                    commit_token(BasicTokenType::ColonRightSquareBracket);
                                    break;
                                default:
                                    commit_token(BasicTokenType::CollateSingleElement);
                                    break;
                            }
                            break;
                        default:
                            consume();
                            commit_token(BasicTokenType::CollateSingleElement);
                            break;
                    }
                }
                if (peek() == '\0') {
                    // Mis matched square bracket
                    m_error_code = REG_EBRACK;
                    return false;
                }
                consume();
                commit_token(BasicTokenType::RightSquareBracket);
                break;
            case '.':
                consume();
                if (prev_was_backslash) {
                    commit_token(BasicTokenType::QuotedCharacter);
                } else {
                    commit_token(BasicTokenType::Period);
                }
                break;
            case '*':
                consume();
                if (prev_was_backslash) {
                    commit_token(BasicTokenType::QuotedCharacter);
                } else if (m_tokens.empty() || m_tokens.last().type() == BasicTokenType::LeftAnchor ||
                           m_tokens.last().type() == BasicTokenType::BackSlashLeftParenthesis) {
                    commit_token(BasicTokenType::OrdinaryCharacter);
                } else {
                    commit_token(BasicTokenType::Asterisk);
                }
                break;
            case '$':
                consume();
                if (prev_was_backslash) {
                    commit_token(BasicTokenType::QuotedCharacter);
                } else if (peek() == '\0') {
                    commit_token(BasicTokenType::RightAnchor);
                } else {
                    commit_token(BasicTokenType::OrdinaryCharacter);
                }
                break;
            case ',':
                consume();
                if (m_tokens.last().type() == BasicTokenType::DuplicateCount) {
                    commit_token(BasicTokenType::Comma);
                } else {
                    commit_token(BasicTokenType::OrdinaryCharacter);
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
                        commit_token(BasicTokenType::BackReference);
                    }
                } else if (m_tokens.last().type() == BasicTokenType::BackSlashLeftCurlyBrace ||
                           m_tokens.last().type() == BasicTokenType::Comma) {
                    while (isdigit(peek())) {
                        consume();
                    }
                    commit_token(BasicTokenType::DuplicateCount);
                } else {
                    commit_token(BasicTokenType::OrdinaryCharacter);
                }
                break;
            case '(':
                consume();
                if (prev_was_backslash) {
                    commit_token(BasicTokenType::BackSlashLeftParenthesis);
                    printf("%lu: %d\n", m_position - 2, m_group_count + 1);
                    m_group_incidices.put(m_position - 2, ++m_group_count);
                } else {
                    commit_token(BasicTokenType::OrdinaryCharacter);
                }
                break;
            case ')':
                consume();
                if (prev_was_backslash) {
                    commit_token(BasicTokenType::BackSlashRightParenthesis);
                } else {
                    commit_token(BasicTokenType::OrdinaryCharacter);
                }
                break;
            case '{':
                consume();
                if (prev_was_backslash) {
                    commit_token(BasicTokenType::BackSlashLeftCurlyBrace);
                } else {
                    commit_token(BasicTokenType::OrdinaryCharacter);
                }
                break;
            case '}':
                consume();
                if (prev_was_backslash) {
                    commit_token(BasicTokenType::BackSlashRightCurlyBrace);
                } else {
                    commit_token(BasicTokenType::OrdinaryCharacter);
                }
                break;
            default:
                consume();
                if (prev_was_backslash) {
                    commit_token(BasicTokenType::QuotedCharacter);
                } else {
                    commit_token(BasicTokenType::OrdinaryCharacter);
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