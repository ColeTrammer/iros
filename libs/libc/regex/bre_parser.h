#pragma once

#include <regex.h>

#include "bre_lexer.h"
#include "bre_value.h"
#include "generic_basic_parser.h"

class BREParser final : public GenericBasicParser<BREValue> {
public:
    using Token = GenericToken<BasicTokenType, BREValue>;

    BREParser(BRELexer& lexer, int cflags) : GenericBasicParser<BREValue>(lexer), m_flags(cflags) {}

    BRELexer& lexer() { return static_cast<BRELexer&>(this->GenericParser::lexer()); }
    const BRELexer& lexer() const { return const_cast<BREParser&>(*this).lexer(); }

    int error_code() const { return m_error_code; }

    const BREValue& result() const { return peek_value_stack(); };

    virtual BREValue reduce_re_dupl_symbol$asterisk(BREValue& v) override {
        assert(v.is<TokenInfo>());
        return { DuplicateCount { DuplicateCount::Type::Star, 0, __INT_MAX__ } };
    }

private:
    virtual void on_error(BasicTokenType) override { m_error_code = REG_BADPAT; }

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
        return { SharedPtr<BRESingleExpression>(
            new BRESingleExpression { BRESingleExpression::Type::OrdinaryCharacter, *ch.as<TokenInfo>().text.start(), {} }) };
    }

    virtual BREValue reduce_nondupl_re$one_char_or_coll_elem_re(BREValue& se) override {
        assert(se.is<SharedPtr<BRESingleExpression>>());
        return se;
    }

    virtual BREValue reduce_nondupl_re$backreference(BREValue& v) override {
        assert(v.is<TokenInfo>());
        int group_index = static_cast<int>(*v.as<TokenInfo>().text.start() - '0');
        if (group_index > lexer().group_at_position(v.as<TokenInfo>().position)) {
            m_error_code = REG_ESUBREG;
            set_error();
            return {};
        }
        return { SharedPtr<BRESingleExpression>(new BRESingleExpression { BRESingleExpression::Type::Backreference, group_index, {} }) };
    }

    virtual BREValue reduce_nondupl_re$backslashleftparenthesis_re_expression_backslashrightparenthesis(BREValue& a, BREValue& v,
                                                                                                        BREValue&) override {
        assert(a.is<TokenInfo>());
        assert(v.is<BRExpression>());
        v.as<BRExpression>().index = lexer().group_at_position(a.as<TokenInfo>().position);
        return { SharedPtr<BRESingleExpression>(
            new BRESingleExpression { BRESingleExpression::Type::Group, move(v.as<BRExpression>()), {} }) };
    }

    virtual BREValue reduce_simple_re$nondupl_re(BREValue& v) override {
        assert(v.is<SharedPtr<BRESingleExpression>>());
        return v;
    }

    virtual BREValue reduce_simple_re$nondupl_re_re_dupl_symbol(BREValue& v, BREValue& c) override {
        assert(v.is<SharedPtr<BRESingleExpression>>());
        assert(c.is<DuplicateCount>());
        v.as<SharedPtr<BRESingleExpression>>()->duplicate = { move(c.as<DuplicateCount>()) };
        return v;
    }

    virtual BREValue reduce_re_expression$simple_re(BREValue& v) override {
        assert(v.is<SharedPtr<BRESingleExpression>>());
        Vector<SharedPtr<BRESingleExpression>> exps;
        exps.add(v.as<SharedPtr<BRESingleExpression>>());
        return { BRExpression { move(exps), 0 } };
    }

    virtual BREValue reduce_re_expression$re_expression_simple_re(BREValue& exps, BREValue& v) override {
        assert(exps.is<BRExpression>());
        assert(v.is<SharedPtr<BRESingleExpression>>());
        exps.as<BRExpression>().parts.add(v.as<SharedPtr<BRESingleExpression>>());
        return exps;
    }

    virtual BREValue reduce_basic_reg_exp$re_expression(BREValue& v) override {
        assert(v.is<BRExpression>());
        return { BRE { false, false, move(v.as<BRExpression>()) } };
    }

    virtual BREValue reduce_basic_reg_exp$leftanchor_re_expression(BREValue&, BREValue& v) override {
        assert(v.is<BRExpression>());
        return { BRE { true, false, move(v.as<BRExpression>()) } };
    }

    virtual BREValue reduce_basic_reg_exp$re_expression_rightanchor(BREValue& v, BREValue&) override {
        assert(v.is<BRExpression>());
        return { BRE { false, true, move(v.as<BRExpression>()) } };
    }

    virtual BREValue reduce_basic_reg_exp$leftanchor_re_expression_rightanchor(BREValue&, BREValue& v, BREValue&) override {
        assert(v.is<BRExpression>());
        return { BRE { true, true, move(v.as<BRExpression>()) } };
    }

    virtual BREValue reduce_basic_reg_exp$leftanchor(BREValue&) override {
        return { BRE { true, false, BRExpression { Vector<SharedPtr<BRESingleExpression>> {}, 0 } } };
    }

    virtual BREValue reduce_basic_reg_exp$rightanchor(BREValue&) override {
        return { BRE { false, true, BRExpression { Vector<SharedPtr<BRESingleExpression>> {}, 0 } } };
    }

    virtual BREValue reduce_basic_reg_exp$leftanchor_rightanchor(BREValue&, BREValue&) override {
        return { BRE { true, true, BRExpression { Vector<SharedPtr<BRESingleExpression>> {}, 0 } } };
    }

private:
    int m_error_code { 0 };
    int m_flags { 0 };
};

struct BRECompiledData {
    BRELexer lexer;
    BREParser parser;
    int cflags;

    BRECompiledData(const char* str, int flags) : lexer(str, flags), parser(lexer, flags) {}
};