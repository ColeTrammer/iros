#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/sync_wait.h>
#include <di/execution/algorithm/then.h>
#include <di/execution/context/run_loop.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/sequence/empty_sequence.h>
#include <di/execution/sequence/ignore_all.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/sequence/transform_each.h>
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

    using Receiver =
        ex::ignore_all_ns::Receiver<decltype(empty), ex::sync_wait_ns::Receiver<di::Result<void>, ex::RunLoop<>>>;

    static_assert(di::concepts::SubscriberOf<Receiver, di::CompletionSignatures<di::SetValue()>>);
}

static void ignore_all() {
    auto empty = ex::empty_sequence();

    ASSERT(ex::sync_wait(ex::ignore_all(empty)));
    ASSERT(ex::sync_wait(ex::ignore_all(ex::just())));
    ASSERT(ex::sync_wait(ex::ignore_all(ex::just(42))));
    ASSERT(ex::sync_wait(ex::ignore_all(ex::just(42, 43))));

    auto was_called = false;
    ASSERT(ex::sync_wait(ex::ignore_all(ex::just() | ex::then([&] {
                                            was_called = true;
                                        }))));
    ASSERT(was_called);

    ASSERT_EQ(ex::sync_wait(ex::ignore_all(ex::just_error(di::Error(di::BasicError::InvalidArgument)))),
              di::Unexpected(di::BasicError::InvalidArgument));
}

static void transform_each() {
    int counter = 0;

    ASSERT(ex::empty_sequence() | ex::transform_each([&](auto&& sender) {
               ++counter;
               return sender;
           }) |
           ex::ignore_all | ex::sync_wait);
    ASSERT_EQ(counter, 0);

    ASSERT(ex::just() | ex::transform_each([&](auto&& sender) {
               ++counter;
               return sender;
           }) |
           ex::ignore_all | ex::sync_wait);
    ASSERT_EQ(counter, 1);
}

TEST(execution_sequence, meta)
TEST(execution_sequence, ignore_all)
TEST(execution_sequence, transform_each)
}
