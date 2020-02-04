#pragma once

#include <liim/string_view.h>
#include <liim/variant.h>

struct TokenInfo {
    StringView text;
    size_t position;
};

using BREValue = Variant<Monostate, TokenInfo>;