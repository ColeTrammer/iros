#include <di/prelude.h>
#include <dius/test/prelude.h>

namespace sync_in_place_stop_source {
static void basic() {
    static_assert(di::concepts::StoppableToken<di::InPlaceStopToken>);
    static_assert(di::concepts::StoppableTokenFor<di::InPlaceStopToken, di::Identity>);

    auto source = di::InPlaceStopSource {};

    auto token = source.get_stop_token();

    bool did_happen1 = false;
    bool did_happen2 = false;

    auto callback = di::InPlaceStopCallback(token, [&] {
        did_happen1 = true;
    });

    {
        auto callback2 = di::InPlaceStopCallback(token, [&] {
            did_happen2 = true;
        });
    }

    struct ErasedDeleter {
        void* obj;
        void (*deleter)(void*);
    };

    ErasedDeleter xx;

    auto bad_cb = new di::InPlaceStopCallback(token, [&] {
        xx.deleter(xx.obj);
    });

    xx.obj = bad_cb;

    xx.deleter = [](void* ptr) {
        return delete reinterpret_cast<decltype(bad_cb)>(ptr);
    };

    ASSERT(!source.stop_requested());
    ASSERT(source.request_stop());
    ASSERT(!source.request_stop());
    ASSERT(source.stop_requested());
    ASSERT(token.stop_possible());
    ASSERT(token.stop_requested());
    ASSERT(did_happen1);
    ASSERT(!did_happen2);
}

TEST(sync_in_place_stop_source, basic)
}
