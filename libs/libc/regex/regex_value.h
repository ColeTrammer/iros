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

struct BracketSingleExpression {
    enum class Type { EquivalenceClass, CharacterClass, GroupedCollatingSymbol, SingleCollatingSymbol };

    Type type;
    StringView expression;
};

struct BracketRangeExpression {
    StringView start;
    StringView end;
};

struct BracketItem {
    enum class Type { SingleExpression, RangeExpression };

    Type type;
    Variant<BracketSingleExpression, BracketRangeExpression> expression;
};

struct BracketExpression {
    Vector<BracketItem> list;
    bool inverted;
};
struct DuplicateCount {
    enum class Type { Exact, AtLeast, Between };

    Type type;
    int min;
    int max;
};

struct RegexSingleExpression;

struct RegexExpression {
    Vector<SharedPtr<RegexSingleExpression>> parts;
};

struct ParsedRegex {
    Vector<RegexExpression> alternatives;
    int index;
};

struct RegexSingleExpression {
    enum class Type { OrdinaryCharacter, QuotedCharacter, Any, BracketExpression, Backreference, Group, LeftAnchor, RightAnchor };
    Type type;
    Variant<char, BracketExpression, int, ParsedRegex> expression;
    Maybe<DuplicateCount> duplicate;
};

using RegexValue = Variant<Monostate, TokenInfo, DuplicateCount, SharedPtr<RegexSingleExpression>, RegexExpression, ParsedRegex,
                           BracketExpression, BracketItem, BracketRangeExpression, BracketSingleExpression>;

inline void dump(const RegexValue& value) {
    const_cast<RegexValue&>(value).visit([](auto&& v) {
        using T = LIIM::decay_t<decltype(v)>;
        using LIIM::IsSame;
        if constexpr (IsSame<Monostate, T>::value) {
            fprintf(stderr, "Invalid value\n");
        } else if constexpr (IsSame<TokenInfo, T>::value) {
            fprintf(stderr, "Token: %lu %s\n", v.position, String(v.text).string());
        } else if constexpr (IsSame<DuplicateCount, T>::value) {
            const DuplicateCount& dup = v;
            switch (dup.type) {
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
        } else if constexpr (IsSame<SharedPtr<RegexSingleExpression>, T>::value) {
            const RegexSingleExpression& exp = *v;
            switch (exp.type) {
                case RegexSingleExpression::Type::Any:
                    fprintf(stderr, "  Any\n");
                    break;
                case RegexSingleExpression::Type::OrdinaryCharacter:
                    fprintf(stderr, "  %c\n", exp.expression.as<char>());
                    break;
                case RegexSingleExpression::Type::QuotedCharacter:
                    fprintf(stderr, "  \\%c\n", exp.expression.as<char>());
                    break;
                case RegexSingleExpression::Type::BracketExpression:
                    fprintf(stderr, "  []\n");
                    break;
                case RegexSingleExpression::Type::Backreference:
                    fprintf(stderr, "  \\%d\n", exp.expression.as<int>());
                    break;
                case RegexSingleExpression::Type::LeftAnchor:
                    fprintf(stderr, "  ^\n");
                    break;
                case RegexSingleExpression::Type::RightAnchor:
                    fprintf(stderr, "  $\n");
                    break;
                case RegexSingleExpression::Type::Group:
                    fprintf(stderr, "  - Group -\n");
                    dump({ exp.expression.as<ParsedRegex>() });
                    fprintf(stderr, "  - End -\n");
                    break;
            }
            if (exp.duplicate.has_value()) {
                dump({ exp.duplicate.value() });
            }
        } else if constexpr (IsSame<RegexExpression, T>::value) {
            const Vector<SharedPtr<RegexSingleExpression>> exps = v.parts;
            fprintf(stderr, "RegexExpression\n");
            exps.for_each([&](const auto& exp) {
                dump({ exp });
            });
        } else if constexpr (IsSame<ParsedRegex, T>::value) {
            const ParsedRegex& regex = v;
            fprintf(stderr, "Regex: %d\n", regex.index);
            regex.alternatives.for_each([&](const auto& a) {
                dump({ a });
            });
        }
    });
}
