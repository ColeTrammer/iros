#pragma once

#include <liim/format/format_parse_context.h>
#include <liim/maybe.h>

namespace LIIM::Format {
namespace Detail {
    enum class Align {
        None,
        Left,
        Center,
        Right,
    };

    enum class Sign {
        MinusOrPlus,
        MinusOrSpace,
        OnlyMinus,
    };

    struct LengthSpecifier {
        int value;
    };

    class BaseOptions {
    public:
    private:
        Maybe<char> m_fill;
        Align m_align { Align::None };
        Sign m_sign { Sign::OnlyMinus };
        bool m_alternate_form { false };
        bool m_zero { false };
        Maybe<LengthSpecifier> m_width;
        Maybe<LengthSpecifier> m_precision;
    };
}

struct BaseFormatter {
    Detail::BaseOptions options;

    constexpr void parse(Parse)
};
}
