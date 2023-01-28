#include <liim/utf8_view.h>
#include <test/test.h>
#include <unicode/east_asian_width.h>
#include <unicode/terminal_width.h>

TEST(terminal_width, basic) {
    EXPECT_EQ(Unicode::terminal_code_point_width('\0'), 1u);
    EXPECT_EQ(Unicode::terminal_code_point_width('a'), 1u);
    EXPECT_EQ(Unicode::terminal_code_point_width(L'한'), 2u);
    EXPECT_EQ(Unicode::terminal_code_point_width(L'！'), 2u);

    EXPECT_EQ(Unicode::terminal_width(Utf8View { "한글" }), 4u);
    EXPECT_EQ(Unicode::terminal_width(Utf8View { "a한b글c" }), 7u);
    EXPECT_EQ(Unicode::terminal_width(Utf8View { "a한b글c！" }), 9u);
}
