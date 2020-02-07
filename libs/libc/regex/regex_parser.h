#pragma once

#include <regex.h>

#include "generic_regex_parser.h"
#include "regex_lexer.h"
#include "regex_value.h"

class RegexParser final : public GenericRegexParser<RegexValue> {
public:
    using Token = GenericToken<RegexTokenType, RegexValue>;

    RegexParser(RegexLexer& lexer, int cflags) : GenericRegexParser<RegexValue>(lexer), m_flags(cflags) {}

    RegexLexer& lexer() { return static_cast<RegexLexer&>(this->GenericParser::lexer()); }
    const RegexLexer& lexer() const { return const_cast<RegexParser&>(*this).lexer(); }

    int error_code() const { return m_error_code; }

    const RegexValue& result() const { return peek_value_stack(); };

    virtual RegexValue reduce_duplicate_symbol$asterisk(RegexValue& v) override {
        assert(v.is<TokenInfo>());
        return { DuplicateCount { DuplicateCount::Type::Star, 0, __INT_MAX__ } };
    }

private:
    virtual void on_error(RegexTokenType) override { m_error_code = REG_BADPAT; }

    virtual RegexValue reduce_duplicate_symbol$leftcurlybrace_duplicatecount_rightcurlybrace(RegexValue&, RegexValue& c,
                                                                                             RegexValue&) override {
        assert(c.is<TokenInfo>());
        int min;
        // NOTE: sscanf is safe, it will stop reading once ints stop and this must be terminated
        //       eventually by either a '\0', ',', or '}' in all cases.
        if (sscanf(c.as<TokenInfo>().text.start(), "%i", &min) != 1) {
            this->set_error();
            m_error_code = REG_BADBR;
            return {};
        }
        return { DuplicateCount { DuplicateCount::Type::Exact, min, min } };
    }

    virtual RegexValue reduce_duplicate_symbol$leftcurlybrace_duplicatecount_comma_rightcurlybrace(RegexValue&, RegexValue& c, RegexValue&,
                                                                                                   RegexValue&) override {
        assert(c.is<TokenInfo>());
        int min;
        // NOTE: sscanf is safe, it will stop reading once ints stop and this must be terminated
        //       eventually by either a '\0', ',', or '}' in all cases.
        if (sscanf(c.as<TokenInfo>().text.start(), "%i", &min) != 1) {
            this->set_error();
            m_error_code = REG_BADBR;
            return {};
        }
        return { DuplicateCount { DuplicateCount::Type::AtLeast, min, __INT_MAX__ } };
    }

    virtual RegexValue
    reduce_duplicate_symbol$leftcurlybrace_duplicatecount_comma_duplicatecount_rightcurlybrace(RegexValue&, RegexValue& c, RegexValue&,
                                                                                               RegexValue& d, RegexValue&) override {
        assert(c.is<TokenInfo>());
        assert(d.is<TokenInfo>());
        int min;
        int max;
        // NOTE: sscanf is safe, it will stop reading once ints stop and this must be terminated
        //       eventually by either a '\0', ',', or '}' in all cases.
        if (sscanf(c.as<TokenInfo>().text.start(), "%d", &min) != 1) {
            this->set_error();
            m_error_code = REG_BADBR;
            return {};
        }
        if (sscanf(d.as<TokenInfo>().text.start(), "%d", &max) != 1) {
            this->set_error();
            m_error_code = REG_BADBR;
            return {};
        }
        return { DuplicateCount { DuplicateCount::Type::Between, min, max } };
    }

    virtual RegexValue reduce_regex_one_char$ordinarycharacter(RegexValue& ch) override {
        assert(ch.is<TokenInfo>());
        return { SharedPtr<RegexSingleExpression>(
            new RegexSingleExpression { RegexSingleExpression::Type::OrdinaryCharacter, { *ch.as<TokenInfo>().text.start() }, {} }) };
    }

    virtual RegexValue reduce_regex_one_char$quotedcharacter(RegexValue& ch) override {
        assert(ch.is<TokenInfo>());
        return { SharedPtr<RegexSingleExpression>(
            new RegexSingleExpression { RegexSingleExpression::Type::QuotedCharacter, { *ch.as<TokenInfo>().text.start() }, {} }) };
    }

    virtual RegexValue reduce_regex_one_char$period(RegexValue&) override {
        return { SharedPtr<RegexSingleExpression>(new RegexSingleExpression { RegexSingleExpression::Type::Any, { '.' }, {} }) };
    }

    virtual RegexValue reduce_expression$regex_one_char(RegexValue& se) override {
        assert(se.is<SharedPtr<RegexSingleExpression>>());
        return se;
    }

    virtual RegexValue reduce_expression$backreference(RegexValue& v) override {
        assert(v.is<TokenInfo>());
        int group_index = static_cast<int>(*v.as<TokenInfo>().text.start() - '0');
        if (group_index > lexer().group_at_position(v.as<TokenInfo>().position)) {
            m_error_code = REG_ESUBREG;
            set_error();
            return {};
        }
        return { SharedPtr<RegexSingleExpression>(
            new RegexSingleExpression { RegexSingleExpression::Type::Backreference, group_index, {} }) };
    }

    virtual RegexValue reduce_expression$leftparenthesis_regex_rightparenthesis(RegexValue& a, RegexValue& v, RegexValue&) override {
        assert(a.is<TokenInfo>());
        assert(v.is<ParsedRegex>());
        v.as<ParsedRegex>().index = lexer().group_at_position(a.as<TokenInfo>().position);
        return { SharedPtr<RegexSingleExpression>(
            new RegexSingleExpression { RegexSingleExpression::Type::Group, move(v.as<RegexExpression>()), {} }) };
    }

    virtual RegexValue reduce_expression$expression_duplicate_symbol(RegexValue& v, RegexValue& c) override {
        assert(v.is<SharedPtr<RegexSingleExpression>>());
        assert(c.is<DuplicateCount>());
        if (v.as<SharedPtr<RegexSingleExpression>>()->duplicate.has_value()) {
            m_error_code = REG_BADPAT;
            set_error();
            return {};
        }
        v.as<SharedPtr<RegexSingleExpression>>()->duplicate = { move(c.as<DuplicateCount>()) };
        return v;
    }

    virtual RegexValue reduce_regex_branch$expression(RegexValue& v) override {
        assert(v.is<SharedPtr<RegexSingleExpression>>());
        Vector<SharedPtr<RegexSingleExpression>> exps;
        exps.add(v.as<SharedPtr<RegexSingleExpression>>());
        return { RegexExpression { move(exps) } };
    }

    virtual RegexValue reduce_regex_branch$regex_branch_expression(RegexValue& exps, RegexValue& v) override {
        assert(exps.is<RegexExpression>());
        assert(v.is<SharedPtr<RegexSingleExpression>>());
        exps.as<RegexExpression>().parts.add(v.as<SharedPtr<RegexSingleExpression>>());
        return exps;
    }

    virtual RegexValue reduce_regex$regex_branch(RegexValue& v) override {
        assert(v.is<RegexExpression>());
        Vector<RegexExpression> alts;
        alts.add(move(v.as<RegexExpression>()));
        return { ParsedRegex { move(alts), 0 } };
    }

    virtual RegexValue reduce_regex$regex_pipe_regex_branch(RegexValue& exps, RegexValue&, RegexValue& v) override {
        assert(exps.is<ParsedRegex>());
        assert(v.is<RegexExpression>());
        exps.as<ParsedRegex>().alternatives.add(move(v.as<RegexExpression>()));
        return exps;
    }

private:
    int m_error_code { 0 };
    int m_flags { 0 };
};