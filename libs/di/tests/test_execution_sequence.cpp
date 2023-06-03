#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/sync_wait.h>
#include <di/execution/algorithm/then.h>
#include <di/execution/algorithm/when_all.h>
#include <di/execution/any/any_sender.h>
#include <di/execution/context/run_loop.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/sequence/empty_sequence.h>
#include <di/execution/sequence/from_container.h>
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

    ASSERT(di::Array { 1, 2, 3, 4 } | ex::from_container | ex::ignore_all | ex::sync_wait);
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

    counter = 0;

    auto nums = di::Array { 1, 2, 3, 4 };
    ASSERT(nums | ex::from_container | ex::transform_each([&](auto&& sender) {
               ++counter;
               return sender;
           }) |
           ex::ignore_all | ex::sync_wait);
    ASSERT_EQ(counter, 4);
}

static void from_container() {
    // Check that from_container() stops if the next senders requests it.
    auto nums = di::Array { 1, 2, 3, 4 };
    auto counter = 0;
    auto result = nums | ex::from_container | ex::transform_each([&](auto&& sender) -> di::AnySenderOf<int> {
                      ++counter;
                      if (counter == 2) {
                          return ex::just_stopped();
                      }
                      return di::forward<decltype(sender)>(sender);
                  }) |
                  ex::ignore_all | ex::sync_wait;
    ASSERT_EQ(result, di::Unexpected(di::BasicError::OperationCanceled));
    ASSERT_EQ(counter, 2);

    // Check that from_container() listens for stop signals from a stop token.
    counter = 0;
    di::Sender auto sender =
        ex::when_all(ex::just_stopped(), nums | ex::from_container | ex::transform_each([&](auto&& sender) {
                                             ++counter;
                                             return sender;
                                         }) | ex::ignore_all);

    ASSERT_EQ(ex::sync_wait(sender), di::Unexpected(di::BasicError::OperationCanceled));
    ASSERT_EQ(counter, 0);

    // Check that views can be used if the user is explicit about it.
    counter = 0;
    ASSERT(ex::from_container(ex::valid_lifetime, "hello"_sv) | ex::transform_each([&](auto&& sender) {
               ++counter;
               return sender;
           }) |
           ex::ignore_all | ex::sync_wait);
    ASSERT_EQ(counter, 5);
}

TEST(execution_sequence, meta)
TEST(execution_sequence, ignore_all)
TEST(execution_sequence, transform_each)
TEST(execution_sequence, from_container)
}
