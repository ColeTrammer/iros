#include "di/vocab/array/array.h"
#include "dius/test/prelude.h"
#include "ttx/paste_event.h"
#include "ttx/paste_event_io.h"

namespace paste_event_io {
static void serialize() {
    using namespace ttx;

    struct Case {
        PasteEvent event;
        di::StringView expected {};
        BracketedPasteMode mode { BracketedPasteMode::Enabled };
    };

    auto cases = di::Array {
        Case { PasteEvent("asdf"_s), "\033[200~asdf\033[201~"_sv },
        Case { PasteEvent("asdf"_s), "asdf"_sv, BracketedPasteMode::Disabled },
    };

    for (auto const& test_case : cases) {
        auto result = serialize_paste_event(test_case.event, test_case.mode);
        ASSERT_EQ(test_case.expected, result);
    }
}

TEST(paste_event_io, serialize)
}
