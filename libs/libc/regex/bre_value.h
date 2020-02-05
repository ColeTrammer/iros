#pragma once

#include <liim/maybe.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <liim/string_view.h>
#include <liim/variant.h>
#include <liim/vector.h>
#include <stdio.h>

struct TokenInfo {
    StringView text;
    size_t position;
};

struct BracketExpression {};
struct DuplicateCount {
    enum class Type { Star, Exact, AtLeast, Between };

    Type type;
    int min;
    int max;
};

struct BRESingleExpression;

struct BRExpression {
    Vector<SharedPtr<BRESingleExpression>> parts;
    int index;
};

struct BRESingleExpression {
    enum class Type { OrdinaryCharacter, QuotedCharacter, Any, BracketExpression, Backreference, Group };
    Type type;
    Variant<char, BracketExpression, int, BRExpression> expression;
    Maybe<DuplicateCount> duplicate;
};

struct BRE {
    bool left_anchor : 1;
    bool right_anchor : 1;
    BRExpression expression;
};

using BREValue = Variant<Monostate, TokenInfo, DuplicateCount, SharedPtr<BRESingleExpression>, BRExpression, BRE>;

inline void dump(const BREValue& value) {
    const_cast<BREValue&>(value).visit([](auto&& v) {
        using T = LIIM::decay_t<decltype(v)>;
        using LIIM::IsSame;
        if constexpr (IsSame<Monostate, T>::value) {
            fprintf(stderr, "Invalid value\n");
        } else if constexpr (IsSame<TokenInfo, T>::value) {
            fprintf(stderr, "Token: %lu %s\n", v.position, String(v.text).string());
        } else if constexpr (IsSame<DuplicateCount, T>::value) {
            const DuplicateCount& dup = v;
            switch (dup.type) {
                case DuplicateCount::Type::Star:
                    fprintf(stderr, "  DUPL: *\n");
                    break;
                case DuplicateCount::Type::AtLeast:
                    fprintf(stderr, "  DUPL: AtLeast %d\n", dup.min);
                    break;
                case DuplicateCount::Type::Exact:
                    fprintf(stderr, "  DUPL: Exactly %d\n", dup.min);
                    break;
                case DuplicateCount::Type::Between:
                    fprintf(stderr, "  DUPL: Between %d %d\n", dup.min, dup.max);
                    break;
            }
        } else if constexpr (IsSame<SharedPtr<BRESingleExpression>, T>::value) {
            const BRESingleExpression& exp = *v;
            switch (exp.type) {
                case BRESingleExpression::Type::Any:
                    fprintf(stderr, "  Any\n");
                    break;
                case BRESingleExpression::Type::OrdinaryCharacter:
                    fprintf(stderr, "  %c\n", exp.expression.as<char>());
                    break;
                case BRESingleExpression::Type::QuotedCharacter:
                    fprintf(stderr, "  \\%c\n", exp.expression.as<char>());
                    break;
                case BRESingleExpression::Type::BracketExpression:
                    fprintf(stderr, "  []\n");
                    break;
                case BRESingleExpression::Type::Backreference:
                    fprintf(stderr, "  \\%d\n", exp.expression.as<int>());
                    break;
                case BRESingleExpression::Type::Group:
                    fprintf(stderr, "  - Group -\n");
                    dump({ exp.expression.as<BRExpression>() });
                    fprintf(stderr, "  - End -\n");
                    break;
            }
            if (exp.duplicate.has_value()) {
                dump({ exp.duplicate.value() });
            }
        } else if constexpr (IsSame<BRExpression, T>::value) {
            const Vector<SharedPtr<BRESingleExpression>> exps = v.parts;
            fprintf(stderr, "BRExpression: %d\n", v.index);
            exps.for_each([&](const auto& exp) {
                dump({ exp });
            });
        } else {
            fprintf(stderr, "Regex\n");
            if (v.left_anchor) {
                fprintf(stderr, "LeftAnchor\n");
            }
            if (v.right_anchor) {
                fprintf(stderr, "Right Anchor\n");
            }
            dump({ v.expression });
        }
    });
}