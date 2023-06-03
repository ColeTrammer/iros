#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/sync_wait.h>
#include <di/execution/algorithm/then.h>
#include <di/execution/context/run_loop.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/sequence/empty_sequence.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/error/prelude.h>
#include <dius/test/prelude.h>

namespace execution_sequence {
namespace ex = di::execution;

static void meta() {
    auto empty = ex::empty_sequence();

    static_assert(di::concepts::SequenceSender<decltype(empty)>);
    static_assert(di::concepts::SequenceSenderIn<decltype(empty)>);
    static_assert(di::concepts::SequenceSenderIn<decltype(empty)&>);
    static_assert(di::concepts::SequenceSenderIn<decltype(empty)&&>);
}

TEST(execution_sequence, meta)
}
