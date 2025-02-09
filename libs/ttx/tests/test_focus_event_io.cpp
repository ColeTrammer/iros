#include "di/vocab/array/array.h"
#include "dius/test/prelude.h"
#include "ttx/focus_event.h"
#include "ttx/focus_event_io.h"
#include "ttx/params.h"

namespace focus_event_io {
static void serialize() {
    using namespace ttx;

    struct Case {
        FocusEvent event;
        di::Optional<di::StringView> expected {};
        FocusEventMode mode { FocusEventMode::Enabled };
    };

    auto cases = di::Array {
        Case { FocusEvent::focus_in(), "\033[I"_sv },
        Case { FocusEvent::focus_out(), "\033[O"_sv },
        Case { FocusEvent::focus_in(), {}, FocusEventMode::Disabled },
    };

    for (auto const& test_case : cases) {
        auto result = serialize_focus_event(test_case.event, test_case.mode);
        ASSERT_EQ(test_case.expected, result);
    }
}

static void parse() {
    using namespace ttx;

    struct Case {
        CSI csi {};
        di::Optional<FocusEvent> expected {};
    };

    auto cases = di::Array {
        Case { CSI(""_s, {}, U'I'), FocusEvent::focus_in() },
        Case { CSI(""_s, {}, U'O'), FocusEvent::focus_out() },
        Case { CSI(""_s, {}, U'o'), {} },
        Case { CSI("$"_s, {}, U'O'), {} },
    };

    for (auto const& test_case : cases) {
        auto result = focus_event_from_csi(test_case.csi);
        ASSERT_EQ(test_case.expected, result);
    }
}

TEST(focus_event_io, serialize)
TEST(focus_event_io, parse)
}
