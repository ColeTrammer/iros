#include "dius/test/prelude.h"
#include "ttx/escape_sequence_parser.h"

namespace escape_sequence_parser {

static void nvim_startup() {
    constexpr auto input =
        "\x1b[?1049h\x1b[22;0;0t\x1b[?1h\x1b=\x1b[H\x1b[2J\x1b[?2004h\x1b[?2026$p\x1b[0m\x1b[4:3m\x1bP$qm\x1b\\\x1b[?u\x1b[c\x1b[?25h"_sv;

    auto expected = di::Array {
        ttx::ParserResult { ttx::CSI("?"_s, { { 1049 } }, 'h') },
        ttx::ParserResult { ttx::CSI(""_s, { { 22 }, { 0 }, { 0 } }, 't') },
        ttx::ParserResult { ttx::CSI("?"_s, { { 1 } }, 'h') },
        ttx::ParserResult { ttx::Escape(""_s, '=') },
        ttx::ParserResult { ttx::CSI(""_s, {}, 'H') },
        ttx::ParserResult { ttx::CSI(""_s, { { 2 } }, 'J') },
        ttx::ParserResult { ttx::CSI("?"_s, { { 2004 } }, 'h') },
        ttx::ParserResult { ttx::CSI("?$"_s, { { 2026 } }, 'p') },
        ttx::ParserResult { ttx::CSI(""_s, { { 0 } }, 'm') },
        ttx::ParserResult { ttx::CSI(""_s, { { 4, 3 } }, 'm') },
        ttx::ParserResult { ttx::DCS("$q"_s, {}, "m"_s) },
        ttx::ParserResult { ttx::CSI("?"_s, {}, 'u') },
        ttx::ParserResult { ttx::CSI(""_s, {}, 'c') },
        ttx::ParserResult { ttx::CSI("?"_s, { { 25 } }, 'h') },
    };

    auto parser = ttx::EscapeSequenceParser {};
    auto actual = parser.parse(input);

    for (auto const& [ex, ac] : di::zip(expected, actual)) {
        ASSERT_EQ(ex, ac);
    }
    ASSERT_EQ(expected.size(), actual.size());
}

TEST(escape_sequence_parser, nvim_startup)
}
