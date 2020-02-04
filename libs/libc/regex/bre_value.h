#pragma once

#include <liim/maybe.h>
#include <liim/string_view.h>
#include <liim/variant.h>
#include <liim/vector.h>

struct TokenInfo {
    StringView text;
    size_t position;
};

struct BracketExpression {};

struct BRESingleExpression {
    enum class Type { OrdinaryCharacter, QuotedCharacter, Any, BracketExpression };
    Variant<char, BracketExpression> expression;
};

struct DuplicateCount {
    enum class Type { Star, Exact, AtLeast, Between };

    Type type;
    int min;
    int max;
};

struct BRExpression {
    Vector<BRESingleExpression>;
    Maybe<DuplicateCount> duplicate;
};

struct BRE {
    bool left_anchor : 1;
    bool right_anchor : 1;
    BRExpression expression;
};

using BREValue = Variant<Monostate, TokenInfo, BRExpression, BRE>;