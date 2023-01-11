#include <di/prelude.h>
#include <test/test.h>

static void basic() {
    static_assert(di::concepts::StoppableToken<di::InPlaceStopToken>);
    static_assert(di::concepts::StoppableTokenFor<di::InPlaceStopToken, di::Identity>);
}

TEST_CONSTEXPRX(sync_in_place_stop_source, basic, basic)