#pragma once

#include "generic_sh_parser.h"
#include "sh_token.h"

class ShParser final : public GenericShParser<ShValue> {
public:
    using Token = GenericShParser<ShValue>::Token;
};