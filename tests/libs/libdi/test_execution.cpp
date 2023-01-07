#include <di/prelude.h>
#include <dius/prelude.h>
#include <test/test.h>

static void meta() {
    auto sender = di::execution::just(5);
    auto sender2 = di::execution::just(5, 10);
    auto sender3 = di::execution::just_error(5);
    auto sender4 = di::execution::just_stopped();

    static_assert(di::SameAs<di::meta::ValueTypesOf<decltype(sender)>, di::Variant<di::Tuple<int>>>);
    static_assert(di::concepts::SenderOf<decltype(sender), int>);
    static_assert(di::SameAs<di::meta::ValueTypesOf<decltype(sender2)>, di::Variant<di::Tuple<int, int>>>);
    static_assert(di::SameAs<di::meta::ValueTypesOf<decltype(sender3)>, di::meta::detail::EmptyVariant>);
    static_assert(di::SameAs<di::meta::ErrorTypesOf<decltype(sender3)>, di::Variant<int>>);
    static_assert(!di::SendsStopped<decltype(sender3)>);
    static_assert(di::SendsStopped<decltype(sender4)>);

    static_assert(di::SameAs<di::meta::Unique<di::meta::List<int, short, int, int>>, di::meta::List<int, short>>);

    using A = di::meta::MakeCompletionSignatures<decltype(sender), di::types::NoEnv,
                                                 di::CompletionSignatures<di::SetValue(i64), di::SetStopped(), di::SetValue(i64)>>;
    static_assert(di::SameAs<A, di::types::CompletionSignatures<di::SetValue(i64), di::SetStopped(), di::SetValue(int)>>);

    static_assert(di::SameAs<di::meta::AsList<di::Tuple<>>, di::meta::List<>>);

    using S = decltype(di::declval<di::RunLoop<>>().get_scheduler());
    using SS = decltype(di::execution::schedule(di::declval<S const&>()));
    static_assert(di::Sender<SS>);
    static_assert(di::TagInvocable<di::Tag<di::execution::get_completion_scheduler<di::SetValue>>, SS const&>);
    static_assert(di::SameAs<S, decltype(di::execution::get_completion_scheduler<di::SetValue>(di::declval<SS const&>()))>);
    static_assert(di::Scheduler<S>);

    static_assert(di::concepts::Awaitable<di::Lazy<i32>>);
    static_assert(di::SameAs<di::CompletionSignatures<di::SetValue(i32), di::SetError(di::Error), di::SetStopped()>,
                             decltype(di::execution::get_completion_signatures(di::declval<di::Lazy<i32>>()))>);

    using R = di::meta::Type<di::execution::sync_wait_ns::Receiver<di::execution::sync_wait_ns::ResultType<di::Lazy<i32>>>>;

    static_assert(di::SameAs<di::CompletionSignatures<di::SetValue(i32), di::SetError(di::Error), di::SetStopped()>,
                             di::meta::Type<di::execution::connect_awaitable_ns::CompletionSignatures<di::Lazy<i32>, R>>>);

    static_assert(di::SameAs<di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>,
                             di::meta::Type<di::execution::connect_awaitable_ns::CompletionSignatures<di::Lazy<>, R>>>);

    static_assert(di::concepts::Receiver<R>);
    static_assert(di::concepts::ReceiverOf<R, di::CompletionSignatures<di::SetValue(i32), di::SetError(di::Error), di::SetStopped()>>);
    static_assert(di::concepts::ReceiverOf<R, di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>>);

    static_assert(di::SameAs<void, di::meta::ValueTypesOf<di::Lazy<>, di::types::NoEnv, di::meta::detail::SingleSenderValueTypeHelper,
                                                          di::meta::detail::SingleSenderValueTypeHelper>>);
    static_assert(di::concepts::SingleSender<di::Lazy<i32>>);
    static_assert(di::concepts::SingleSender<di::Lazy<>>);
}

static void sync_wait() {
    namespace ex = di::execution;

    ASSERT_EQ(ex::sync_wait(ex::just(42)), 42);

    ASSERT(ex::sync_wait(ex::get_scheduler()));
    ASSERT(ex::sync_wait(ex::get_stop_token()));
}

static void lazy() {
    constexpr static auto t2 = [] -> di::Lazy<> {
        co_return;
    };

    constexpr static auto task = [] -> di::Lazy<i32> {
        co_await t2();
        co_return 42;
    };

    ASSERT(di::sync_wait(t2()));
    ASSERT_EQ(di::sync_wait(task()), 42);
}

static void coroutine() {
    namespace ex = di::execution;

    constexpr static auto task = [] -> di::Lazy<i32> {
        auto x = co_await ex::just(42);
        co_return x;
    };
    ASSERT_EQ(di::sync_wait(task()), 42);

    constexpr static auto error = [] -> di::Lazy<i32> {
        co_await ex::just_error(di::BasicError::Invalid);
        co_return 56;
    };
    ASSERT_EQ(di::sync_wait(error()), di::Unexpected(di::BasicError::Invalid));

    constexpr static auto stopped = [] -> di::Lazy<i32> {
        co_await ex::just_stopped();
        co_return 56;
    };
    ASSERT_EQ(di::sync_wait(stopped()), di::Unexpected(di::BasicError::Cancelled));
}

static void then() {
    namespace ex = di::execution;

    di::Sender auto work = ex::just(42) | ex::then([](int x) {
                               return x * 2;
                           });

    using S = decltype(work);

    using R = ex::then_ns::Receiver<ex::sync_wait_ns::Receiver<di::Result<di::Tuple<int>>>, di::Identity>;

    static_assert(di::Receiver<R>);

    static_assert(requires { ex::set_value(di::declval<R>(), 1); });
    static_assert(di::ReceiverOf<R, di::CompletionSignatures<di::SetStopped()>>);
    static_assert(di::ReceiverOf<R, di::CompletionSignatures<di::SetValue(i32)>>);
    static_assert(di::SenderTo<S, R>);

    di::Sender auto w2 = ex::just(42) | ex::then(di::into_void);

    ASSERT_EQ(ex::sync_wait(di::move(work)), 84);
    ASSERT(ex::sync_wait(di::move(w2)));
}

static void inline_scheduler() {
    namespace ex = di::execution;

    auto scheduler = di::InlineScheduler {};

    auto work = ex::schedule(scheduler) | ex::then([] {
                    return 42;
                });

    auto w2 = ex::on(scheduler, ex::just(42));

    ASSERT_EQ(ex::sync_wait(di::move(work)), 42);
    ASSERT_EQ(ex::sync_wait(di::move(w2)), 42);
}

TEST_CONSTEXPRX(execution, meta, meta)
TEST_CONSTEXPRX(execution, sync_wait, sync_wait)
TEST_CONSTEXPRX(execution, lazy, lazy)
TEST_CONSTEXPRX(execution, coroutine, coroutine)
TEST_CONSTEXPRX(execution, then, then)
TEST_CONSTEXPRX(execution, inline_scheduler, inline_scheduler)