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

private:
    int m_error_code { 0 };
};