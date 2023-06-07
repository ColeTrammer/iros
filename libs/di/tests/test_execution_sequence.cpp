#include <di/container/view/prelude.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/sync_wait.h>
#include <di/execution/algorithm/then.h>
#include <di/execution/algorithm/when_all.h>
#include <di/execution/any/any_sender.h>
#include <di/execution/context/run_loop.h>
#include <di/execution/query/is_always_lockstep_sequence.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/sequence/async_generator.h>
#include <di/execution/sequence/empty_sequence.h>
#include <di/execution/sequence/from_container.h>
#include <di/execution/sequence/ignore_all.h>
#include <di/execution/sequence/let_each.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/sequence/then_each.h>
#include <di/execution/sequence/transform_each.h>
#include <di/execution/sequence/zip.h>
#include <di/execution/types/prelude.h>
#include <di/function/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/expected/prelude.h>
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

    static_assert(di::concepts::AwaitableAsyncRange<di::AsyncGenerator<int>>);

    auto from_container = ex::from_container(di::Array { 1, 2, 3 });

    static_assert(di::concepts::AlwaysLockstepSequence<decltype(empty)>);
    static_assert(di::concepts::AlwaysLockstepSequence<decltype(from_container)>);
    static_assert(di::concepts::AlwaysLockstepSequence<decltype(from_container | ex::transform_each(di::identity))>);
    static_assert(di::concepts::AlwaysLockstepSequence<decltype(ex::just())>);
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

static void then() {
    auto sum = 0;
    auto sender = ex::from_container(ex::valid_lifetime, di::range(1, 5)) | ex::then_each([&](int x) {
                      sum += x;
                  }) |
                  ex::ignore_all;
    ASSERT(ex::sync_wait(sender));
    ASSERT_EQ(sum, 10);
}

static void let() {
    auto sum = 0;
    auto s1 = ex::from_container(ex::valid_lifetime, di::range(1, 5)) | ex::let_value_each([&](int& x) {
                  sum += x;
                  return ex::just();
              }) |
              ex::ignore_all;
    ASSERT(ex::sync_wait(s1));
    ASSERT_EQ(sum, 10);

    sum = 0;
    auto s2 = ex::just_error(di::Error(di::BasicError::InvalidArgument)) | ex::let_error_each([&](di::Error& e) {
                  ASSERT_EQ(e, di::Error(di::BasicError::InvalidArgument));
                  return ex::just(5);
              }) |
              ex::then_each([&](int x) {
                  sum += x;
              }) |
              ex::ignore_all;
    ASSERT(ex::sync_wait(di::move(s2)));
    ASSERT_EQ(sum, 5);

    sum = 0;
    auto s3 = ex::just_stopped() | ex::let_stopped_each([&] {
                  return ex::just(5);
              }) |
              ex::then_each([&](int x) {
                  sum += x;
              }) |
              ex::ignore_all;
    ASSERT(ex::sync_wait(di::move(s3)));
    ASSERT_EQ(sum, 5);
}

static void async_generator() {
    auto h = [](int x) -> di::Lazy<int> {
        co_return x;
    };

    enum class Outcome { Value, Error, Stopped };

    auto g = [&](Outcome outcome) -> di::AsyncGenerator<int> {
        co_yield co_await h(1);
        co_yield co_await h(2);
        co_yield co_await h(3);
        if (outcome == Outcome::Error) {
            co_return di::Unexpected(di::BasicError::InvalidArgument);
        } else if (outcome == Outcome::Stopped) {
            co_return di::stopped;
        }
        co_return {};
    };

    auto f = [&](Outcome outcome) -> di::Lazy<int> {
        auto sequence = co_await g(outcome);
        auto sum = 0;
        while (auto next = co_await ex::next(sequence)) {
            sum += *next;
        }
        co_return sum;
    };

    ASSERT_EQ(ex::sync_wait(f(Outcome::Value)), 6);
    ASSERT_EQ(ex::sync_wait(f(Outcome::Error)), di::Unexpected(di::BasicError::InvalidArgument));
    ASSERT_EQ(ex::sync_wait(f(Outcome::Stopped)), di::Unexpected(di::BasicError::OperationCanceled));
}

static void zip() {
    namespace execution = di::execution;
    auto sequence = execution::zip(execution::from_container(di::Array { 1, 2, 3 }),
                                   execution::from_container(di::Array { 4, 5, 6 }));

    static_assert(di::concepts::AlwaysLockstepSequence<decltype(sequence)>);

    ASSERT(execution::sync_wait(execution::ignore_all(
        execution::zip(execution::empty_sequence(), execution::empty_sequence(), execution::empty_sequence()))));

    // auto sum = 0;
    // ASSERT(execution::sync_wait(execution::ignore_all(sequence | execution::then_each([&](int x, int y) {
    //                                                       sum += x * y;
    //                                                   }))));
    // ASSERT_EQ(sum, 32);
}

TEST(execution_sequence, meta)
TEST(execution_sequence, ignore_all)
TEST(execution_sequence, transform_each)
TEST(execution_sequence, from_container)
TEST(execution_sequence, then)
TEST(execution_sequence, let)
TEST(execution_sequence, async_generator)
TEST(execution_sequence, zip)
}
