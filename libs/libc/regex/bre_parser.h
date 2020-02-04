#pragma once

#include <regex.h>

#include "bre_value.h"
#include "generic_basic_parser.h"

class BREParser final : public GenericBasicParser<BREValue> {
public:
    using Token = GenericToken<BasicTokenType, BREValue>;

    BREParser(GenericLexer<BasicTokenType, BREValue>& lexer) : GenericBasicParser<BREValue>(lexer) {}

    int error_code() const { return m_error_code; }

    virtual void on_error(BasicTokenType) override { m_error_code = REG_BADPAT; }

    const BREValue& result() const { return peek_value_stack(); };

    virtual BREValue reduce_re_dupl_symbol$asterisk(BREValue& v) override {
        assert(v.is<TokenInfo>());
        return { DuplicateCount { DuplicateCount::Type::Star, 0, __INT_MAX__ } };
    }

    virtual BREValue reduce_re_dupl_symbol$backslashleftcurlybrace_duplicatecount_backslashrightcurlybrace(BREValue&, BREValue& c,
                                                                                                           BREValue&) override {
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

    virtual BREValue reduce_re_dupl_symbol$backslashleftcurlybrace_duplicatecount_comma_backslashrightcurlybrace(BREValue&, BREValue& c,
                                                                                                                 BREValue&,
                                                                                                                 BREValue&) override {
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

    virtual BREValue reduce_re_dupl_symbol$backslashleftcurlybrace_duplicatecount_comma_duplicatecount_backslashrightcurlybrace(
        BREValue&, BREValue& c, BREValue&, BREValue& d, BREValue&) override {
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

    virtual BREValue reduce_one_char_or_coll_elem_re$ordinarycharacter(BREValue& ch) override {
        assert(ch.is<TokenInfo>());
        return { BRESingleExpression { BRESingleExpression::Type::OrdinaryCharacter, *ch.as<TokenInfo>().text.start(), {} } };
    }

    virtual BREValue reduce_nondupl_re$one_char_or_coll_elem_re(BREValue& se) override {
        assert(se.is<BRESingleExpression>());
        return se;
    }

    virtual BREValue reduce_simple_re$nondupl_re(BREValue& v) override {
        assert(v.is<BRESingleExpression>());
        return v;
    }

    virtual BREValue reduce_simple_re$nondupl_re_re_dupl_symbol(BREValue& v, BREValue& c) override {
        assert(v.is<BRESingleExpression>());
        assert(c.is<DuplicateCount>());
        v.as<BRESingleExpression>().duplicate = { move(c.as<DuplicateCount>()) };
        return v;
    }

    virtual BREValue reduce_re_expression$simple_re(BREValue& v) override {
        assert(v.is<BRESingleExpression>());
        Vector<BRESingleExpression> exps;
        exps.add(move(v.as<BRESingleExpression>()));
        return { BRExpression { move(exps) } };
    }

    virtual BREValue reduce_re_expression$re_expression_simple_re(BREValue& exps, BREValue& v) override {
        assert(exps.is<BRExpression>());
        assert(v.is<BRESingleExpression>());
        exps.as<BRExpression>().parts.add(move(v.as<BRESingleExpression>()));
        return exps;
    }

    virtual BREValue reduce_basic_reg_exp$re_expression(BREValue& v) override {
        assert(v.is<BRExpression>());
        return { BRE { false, false, move(v.as<BRExpression>()) } };
    }

private:
    int m_error_code { 0 };
};