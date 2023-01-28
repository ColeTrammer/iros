#include <ext/parser.h>
#include <liim/try.h>
#include <test/test.h>

namespace T {
struct X {
    int a;
    int b;
    int c;
};
}

namespace Ext {
template<>
struct ParserAdapter<T::X> {
    static Result<T::X, ParserError> parse(Parser& parser) {
        auto a = TRY(Ext::parse_partial<int>(parser));
        TRY(parser.consume(":"sv));
        auto b = TRY(Ext::parse_partial<int>(parser));
        TRY(parser.consume(":"sv));
        auto c = TRY(Ext::parse_partial<int>(parser));
        return T::X(a, b, c);
    }
};
}

TEST(parser, basic) {
    auto input = "43:2:1"sv;

    auto result = Ext::parse<T::X>(input);
    if (!result.has_value()) {
        auto message = result.error().message();
        EXPECT_EQ(message, "Not an error");
        EXPECT(false);
    }
    EXPECT_EQ(result.value().a, 43);
    EXPECT_EQ(result.value().b, 2);
    EXPECT_EQ(result.value().c, 1);
}
