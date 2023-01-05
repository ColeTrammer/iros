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
}

static void sync_wait() {
    namespace ex = di::execution;

    ASSERT_EQ(ex::sync_wait(ex::just(42)), 42);

    ASSERT(ex::sync_wait(ex::get_scheduler()));
}

TEST_CONSTEXPRX(execution, meta, meta)
TEST_CONSTEXPRX(execution, sync_wait, sync_wait)