#include <di/prelude.h>
#include <test/test.h>

constexpr void basic() {
    auto x = di::StringView("QWER", 4);
    (void) x;

    EXPECT(x.size() == 4);

    int c = 0;
    for (auto xx : x) {
        (void) xx;
        c++;
    }
    EXPECT(c == 4);
}

constexpr void push_back() {
    auto x = di::String {};
    x.push_back('a');
    x.push_back('b');
    x.push_back('c');
    EXPECT(x.size() == 3);
}

TEST_CONSTEXPR(container_string, basic, basic)
TEST_CONSTEXPR(container_string, push_back, push_back)
