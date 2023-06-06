#include <di/any/storage/hybrid_storage.h>
#include <di/any/storage/prelude.h>
#include <di/any/storage/unique_storage.h>
#include <di/any/vtable/maybe_inline_vtable.h>
#include <di/concepts/prelude.h>
#include <di/container/allocator/allocation_result.h>
#include <di/container/allocator/allocator.h>
#include <di/execution/algorithm/into_result.h>
#include <di/execution/algorithm/into_variant.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/just_or_error.h>
#include <di/execution/algorithm/just_void_or_stopped.h>
#include <di/execution/algorithm/prelude.h>
#include <di/execution/algorithm/sync_wait.h>
#include <di/execution/algorithm/then.h>
#include <di/execution/algorithm/when_all.h>
#include <di/execution/any/any_operation_state.h>
#include <di/execution/any/any_sender.h>
#include <di/execution/concepts/prelude.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/meta/completion_signatures_of.h>
#include <di/execution/meta/sends_stopped.h>
#include <di/execution/prelude.h>
#include <di/execution/query/get_allocator.h>
#include <di/execution/query/get_stop_token.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/types/empty_env.h>
#include <di/execution/types/prelude.h>
#include <di/platform/prelude.h>
#include <di/types/integers.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/expected/prelude.h>
#include <dius/test/prelude.h>

namespace execution {
static void meta() {
    auto sender = di::execution::just(5);
    auto sender2 = di::execution::just(5, 10);
    auto sender3 = di::execution::just_error(5);
    auto sender4 = di::execution::just_stopped();

    static_assert(di::SameAs<di::meta::ValueTypesOf<decltype(sender)>, di::Variant<di::Tuple<int>>>);
    static_assert(di::concepts::SenderOf<decltype(sender), di::SetValue(int)>);
    static_assert(di::SameAs<di::meta::ValueTypesOf<decltype(sender2)>, di::Variant<di::Tuple<int, int>>>);
    static_assert(di::SameAs<di::meta::ValueTypesOf<decltype(sender3)>, di::meta::detail::EmptyVariant>);
    static_assert(di::SameAs<di::meta::ErrorTypesOf<decltype(sender3)>, di::Variant<int>>);
    static_assert(!di::meta::sends_stopped<decltype(sender3)>);
    static_assert(di::meta::sends_stopped<decltype(sender4)>);

    static_assert(di::SameAs<di::meta::Unique<di::meta::List<int, short, int, int>>, di::meta::List<int, short>>);

    using A = di::meta::MakeCompletionSignatures<
        decltype(sender), di::types::EmptyEnv,
        di::CompletionSignatures<di::SetValue(i64), di::SetStopped(), di::SetValue(i64)>>;
    static_assert(
        di::SameAs<A, di::types::CompletionSignatures<di::SetValue(i64), di::SetStopped(), di::SetValue(int)>>);

    static_assert(di::SameAs<di::meta::AsList<di::Tuple<>>, di::meta::List<>>);

    using S = decltype(di::declval<di::RunLoop<>>().get_scheduler());
    using SS = decltype(di::execution::schedule(di::declval<S const&>()));
    using SE = decltype(di::execution::get_env(di::declval<SS const&>()));
    static_assert(!di::SameAs<SE, di::EmptyEnv>);
    static_assert(di::Sender<SS>);
    static_assert(di::TagInvocable<di::Tag<di::execution::get_completion_scheduler<di::SetValue>>, SE const&>);
    static_assert(
        di::SameAs<S, decltype(di::execution::get_completion_scheduler<di::SetValue>(di::declval<SE const&>()))>);
    static_assert(di::Scheduler<S>);

    static_assert(di::concepts::IsAwaitable<di::Lazy<i32>>);
    static_assert(
        di::SameAs<di::CompletionSignatures<di::SetValue(i32), di::SetError(di::Error), di::SetStopped()>,
                   decltype(di::execution::get_completion_signatures(di::declval<di::Lazy<i32>>(), di::EmptyEnv {}))>);

    using R = di::execution::sync_wait_ns::Receiver<
        di::execution::sync_wait_ns::ResultType<di::execution::RunLoop<>, di::Lazy<i32>>, di::execution::RunLoop<>>;

    static_assert(
        di::SameAs<di::CompletionSignatures<di::SetValue(i32), di::SetError(di::Error), di::SetStopped()>,
                   di::meta::Type<di::execution::connect_awaitable_ns::CompletionSignatures<di::Lazy<i32>, R>>>);

    static_assert(di::SameAs<di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>,
                             di::meta::Type<di::execution::connect_awaitable_ns::CompletionSignatures<di::Lazy<>, R>>>);

    static_assert(di::concepts::Receiver<R>);
    static_assert(di::concepts::ReceiverOf<
                  R, di::CompletionSignatures<di::SetValue(i32), di::SetError(di::Error), di::SetStopped()>>);
    static_assert(
        di::concepts::ReceiverOf<R,
                                 di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>>);

    static_assert(
        di::SameAs<
            void, di::meta::ValueTypesOf<di::Lazy<>, di::types::EmptyEnv, di::meta::detail::SingleSenderValueTypeHelper,
                                         di::meta::detail::SingleSenderValueTypeHelper>>);
    static_assert(di::concepts::SingleSender<di::Lazy<i32>, di::EmptyEnv>);
    static_assert(di::concepts::SingleSender<di::Lazy<>, di::EmptyEnv>);

    namespace ex = di::execution;

    constexpr auto env = ex::make_env(di::empty_env, ex::with(ex::get_allocator, di::FallibleAllocator {}));
    static_assert(di::SameAs<di::FallibleAllocator, di::meta::AllocatorOf<decltype(env)>>);
}

static void sync_wait() {
    namespace ex = di::execution;

    ASSERT_EQ(ex::sync_wait(ex::just(42)), 42);

    ASSERT(ex::sync_wait(ex::get_scheduler()));
    ASSERT(ex::sync_wait(ex::get_stop_token()));

    ASSERT_EQ(ex::sync_wait(ex::just_error(di::BasicError::InvalidArgument)),
              di::Unexpected(di::BasicError::InvalidArgument));

    ASSERT_EQ(ex::sync_wait(ex::just_stopped()), di::Unexpected(di::BasicError::OperationCanceled));
}

static void lazy() {
    constexpr static auto t2 = [] -> di::Lazy<> {
        co_return {};
    };

    constexpr static auto task = [] -> di::Lazy<i32> {
        co_await t2();
        co_return 42;
    };

    ASSERT(di::sync_wait(t2()));
    ASSERT_EQ(di::sync_wait(task()), 42);
}

static void just() {
    ASSERT_EQ(di::sync_wait(di::execution::just(42)), 42);
    ASSERT_EQ(di::sync_wait(di::execution::just_error(di::BasicError::InvalidArgument)),
              di::Unexpected(di::BasicError::InvalidArgument));
    ASSERT_EQ(di::sync_wait(di::execution::just_stopped()), di::Unexpected(di::BasicError::OperationCanceled));

    ASSERT_EQ(di::sync_wait(di::execution::just_void_or_stopped(true)),
              di::Unexpected(di::BasicError::OperationCanceled));
    ASSERT(di::sync_wait(di::execution::just_void_or_stopped(false)));

    ASSERT_EQ(di::sync_wait(di::execution::just_or_error(di::Result<int>(42))), 42);
    ASSERT_EQ(
        di::sync_wait(di::execution::just_or_error(di::Result<int>(di::unexpect, di::BasicError::InvalidArgument))),
        di::Unexpected(di::BasicError::InvalidArgument));
}

static void coroutine() {
    namespace ex = di::execution;

    constexpr static auto task = [] -> di::Lazy<i32> {
        auto x = co_await ex::just(42);
        co_return x;
    };
    ASSERT_EQ(di::sync_wait(task()), 42);

    constexpr static auto error = [] -> di::Lazy<i32> {
        co_await ex::just_error(di::BasicError::InvalidArgument);
        co_return 56;
    };
    ASSERT_EQ(di::sync_wait(error()), di::Unexpected(di::BasicError::InvalidArgument));

    constexpr static auto error_direct = [] -> di::Lazy<i32> {
        co_return di::Unexpected(di::BasicError::InvalidArgument);
    };
    ASSERT_EQ(di::sync_wait(error_direct()), di::Unexpected(di::BasicError::InvalidArgument));

    constexpr static auto stopped = [] -> di::Lazy<i32> {
        co_await ex::just_stopped();
        co_return 56;
    };
    ASSERT_EQ(di::sync_wait(stopped()), di::Unexpected(di::BasicError::OperationCanceled));

    constexpr static auto stopped_direct = [] -> di::Lazy<i32> {
        co_return di::stopped;
    };
    ASSERT_EQ(di::sync_wait(stopped_direct()), di::Unexpected(di::BasicError::OperationCanceled));
}

static void then() {
    namespace ex = di::execution;

    di::Sender auto work = ex::just(42) | ex::then([](int x) {
                               return x * 2;
                           });

    using S = decltype(work);

    using R =
        ex::then_ns::Receiver<ex::sync_wait_ns::Receiver<di::Result<di::Tuple<int>>, ex::RunLoop<>>, di::Identity>;

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

    auto v = ex::on(scheduler, ex::just() | ex::let_value([] {
                                   return ex::get_scheduler();
                               }));
    ASSERT_EQ(*ex::sync_wait(di::move(v)), scheduler);
}

static void let() {
    namespace ex = di::execution;

    auto w = ex::just(42) | ex::let_value(ex::just);
    ASSERT_EQ(ex::sync_wait(di::move(w)), 42);

    auto scheduler = di::InlineScheduler {};

    auto v =
        ex::schedule(scheduler) | ex::let_value(ex::get_scheduler) | ex::let_value([](di::Scheduler auto& scheduler) {
            return ex::schedule(scheduler) | ex::then([] {
                       return 43;
                   });
        });
    ASSERT_EQ(ex::sync_wait(di::move(v)), 43);

    auto z = ex::just() | ex::then([] {
                 return di::Result<long>(44);
             }) |
             ex::let_value([](long) {
                 return ex::just(44);
             });

    ASSERT_EQ(ex::sync_wait(di::move(z)), 44);

    auto y = ex::schedule(scheduler) | ex::let_value([] {
                 return ex::just_error(di::BasicError::InvalidArgument);
             }) |
             ex::let_error([](auto) {
                 return ex::just(42);
             }) |
             ex::let_value([](auto) {
                 return ex::just_error(di::BasicError::InvalidArgument);
             }) |
             ex::let_error([](auto) {
                 return ex::just_stopped();
             }) |
             ex::let_stopped([] {
                 return ex::just(42);
             });
    ASSERT_EQ(ex::sync_wait(di::move(y)), 42);

    auto a = ex::let_value_with(
        [] {
            return 42;
        },
        [](int& x) {
            return ex::just(x);
        });

    ASSERT_EQ(ex::sync_wait(di::move(a)), 42);
}

static void transfer() {
    namespace ex = di::execution;

    auto scheduler = di::InlineScheduler {};

    auto w = ex::transfer_just(scheduler, 42);

    ASSERT_EQ(ex::sync_wait(di::move(w)), 42);
}

static void as() {
    namespace ex = di::execution;

    auto w = ex::just_stopped() | ex::stopped_as_optional;
    ASSERT_EQ(ex::sync_wait(di::move(w)), di::nullopt);

    auto v = ex::just_stopped() | ex::stopped_as_error(42) | ex::let_error([](int x) {
                 ASSERT_EQ(x, 42);
                 return ex::just(x);
             });
    ASSERT_EQ(ex::sync_wait(di::move(v)), 42);
}

struct AsyncI32 {
    i32 value;

private:
    friend auto tag_invoke(di::Tag<di::execution::async_create_in_place>, di::InPlaceType<AsyncI32>, i32 value) {
        return di::execution::just(AsyncI32 { value });
    }

    friend auto tag_invoke(di::Tag<di::execution::async_destroy_in_place>, di::InPlaceType<AsyncI32>, AsyncI32&) {
        return di::execution::just();
    }
};

static void with() {
    namespace ex = di::execution;

    auto w = ex::async_create<AsyncI32>(42) | ex::use_resource([](AsyncI32& value) {
                 return ex::just(value.value);
             });

    ASSERT_EQ(ex::sync_wait(di::move(w)), 42);
}

struct FailAllocator {
    friend di::Result<di::AllocationResult<>> tag_invoke(di::Tag<di::allocate>, FailAllocator, usize, usize) {
        return di::Unexpected(di::BasicError::NotEnoughMemory);
    }

    friend void tag_invoke(di::Tag<di::deallocate>, FailAllocator, void*, usize, usize) {}
};

static void any_sender() {
    namespace ex = di::execution;

    using Sigs = di::CompletionSignatures<di::SetValue(int)>;
    using Sender = di::AnySender<Sigs>;
    using Receiver = Sender::Receiver;
    using OperationState = Sender::OperationState;

    static_assert(di::concepts::Receiver<Receiver>);
    static_assert(di::concepts::OperationState<OperationState>);
    static_assert(di::concepts::MoveConstructible<OperationState>);

    static_assert(di::concepts::ReceiverOf<Receiver, Sigs>);

    auto x = Sender(ex::just(42));

    ASSERT_EQ(ex::sync_wait(di::move(x)), 42);

    auto y = Sender(ex::just(42)) | ex::let_value([](int x) {
                 return ex::just(x);
             });

    ASSERT_EQ(ex::sync_wait(di::move(y)), 42);

    using Sender2 = di::AnySender<
        Sigs, void, di::any::HybridStorage<>, di::any::MaybeInlineVTable<3>,
        di::AnyOperationState<void, di::any::HybridStorage<di::StorageCategory::MoveOnly, 8 * sizeof(void*),
                                                           alignof(void*), FailAllocator>>>;

    auto z = ex::just(42) | ex::let_value([](int x) {
                 return ex::just(x);
             });
    auto yy = Sender2(di::move(z));

    ASSERT_EQ(ex::sync_wait(di::move(yy)), di::Unexpected(di::BasicError::NotEnoughMemory));

    using Sender3 = di::AnySender<
        Sigs, void,
        di::any::HybridStorage<di::StorageCategory::MoveOnly, 2 * sizeof(void*), alignof(void*), FailAllocator>>;

    auto dummy = 0;
    auto zz = ex::just(42) | ex::let_value([=](int x) {
                  return ex::just(x + dummy);
              }) |
              ex::let_value([=](int x) {
                  return ex::just(x + dummy);
              }) |
              ex::let_value([=](int x) {
                  return ex::just(x + dummy);
              }) |
              ex::let_value([=](int x) {
                  return ex::just(x + dummy);
              });
    auto yyy = Sender3(di::move(zz));

    ASSERT_EQ(ex::sync_wait(di::move(yyy)), di::Unexpected(di::BasicError::NotEnoughMemory));

    using Sender4 = di::AnySender<di::CompletionSignatures<di::SetValue(), di::SetValue(int), di::SetStopped()>>;

    ASSERT_EQ(ex::sync_wait_with_variant(Sender4(ex::just(52))), di::make_tuple(52));
    ASSERT_EQ(ex::sync_wait_with_variant(Sender4(ex::just())), di::make_tuple());
    ASSERT_EQ(ex::sync_wait_with_variant(Sender4(ex::just_stopped())),
              di::Unexpected(di::BasicError::OperationCanceled));
    ASSERT_EQ(ex::sync_wait_with_variant(Sender4(ex::just_error(di::BasicError::InvalidArgument))),
              di::Unexpected(di::BasicError::InvalidArgument));

    using Sender5 = di::AnySenderOf<int>;

    auto task = [] -> Sender5 {
        auto x = co_await ex::just(42);
        co_return x;
    };
    ASSERT_EQ(di::sync_wait(Sender5(task())), 42);

    auto task2 = [] -> Sender5 {
        auto x = co_await di::Result<int>(42);
        co_return x;
    };
    ASSERT_EQ(di::sync_wait(Sender5(task2())), 42);

    auto task3 = [] -> Sender5 {
        auto x = co_await di::Result<int>(di::Unexpected(di::BasicError::InvalidArgument));
        co_return x;
    };
    ASSERT_EQ(di::sync_wait(Sender5(task3())), di::Unexpected(di::BasicError::InvalidArgument));

    ASSERT_EQ(di::sync_wait(Sender5(di::Unexpected(di::BasicError::InvalidArgument))),
              di::Unexpected(di::BasicError::InvalidArgument));
    ASSERT_EQ(di::sync_wait(Sender5(di::stopped)), di::Unexpected(di::BasicError::OperationCanceled));

    ASSERT_EQ(di::sync_wait(di::Result<int>(42)), 42);
    ASSERT_EQ(di::sync_wait(di::Unexpected(di::BasicError::InvalidArgument)),
              di::Unexpected(di::BasicError::InvalidArgument));
}

static void into_result() {
    namespace ex = di::execution;

    using Sender = di::AnySenderOf<int>;

    enum class Result { Ok, Err, Stopped };

    auto process = [](Sender sender) {
        return di::move(sender) | ex::into_result | ex::then([](di::Result<int> result) {
                   if (result) {
                       return Result::Ok;
                   }
                   if (result.error() == di::BasicError::OperationCanceled) {
                       return Result::Stopped;
                   }
                   return Result::Err;
               });
    };

    ASSERT_EQ(ex::sync_wait(process(ex::just(42))), Result::Ok);
    ASSERT_EQ(ex::sync_wait(process(ex::just_error(di::BasicError::InvalidArgument))), Result::Err);
    ASSERT_EQ(ex::sync_wait(process(ex::just_stopped())), Result::Stopped);
}

static void when_all() {
    namespace ex = di::execution;

    using S1 = decltype(ex::when_all(ex::just(42), ex::just(43, 44L), ex::just()));
    static_assert(di::SameAs<di::CompletionSignatures<di::SetValue(int&&, int&&, long&&), di::SetStopped()>,
                             di::meta::CompletionSignaturesOf<S1>>);

    using S2 = decltype(ex::when_all(ex::just(42), ex::just(43, 44L), ex::just(),
                                     ex::just_error(di::Error(di::BasicError::InvalidArgument))));
    static_assert(di::SameAs<di::CompletionSignatures<di::SetError(di::Error&&), di::SetStopped()>,
                             di::meta::CompletionSignaturesOf<S2>>);

    using S3 =
        decltype(ex::when_all(ex::just(42), ex::just(43, 44L), ex::just(), ex::just_or_error(di::Result<int>(45))));
    static_assert(di::SameAs<di::CompletionSignatures<di::SetValue(int&&, int&&, long&&, int&&),
                                                      di::SetError(di::Error&&), di::SetStopped()>,
                             di::meta::CompletionSignaturesOf<S3>>);

    auto s1 = ex::when_all(ex::just(42), ex::just(43, 44L), ex::just());
    ASSERT_EQ(ex::sync_wait(s1), di::make_tuple(42, 43, 44L));

    auto s2 = ex::when_all(ex::just(42), ex::just(43, 44L), ex::just(),
                           ex::just_error(di::Error(di::BasicError::InvalidArgument)));
    ASSERT_EQ(ex::sync_wait(di::move(s2)), di::Unexpected(di::BasicError::InvalidArgument));

    auto s3 = ex::when_all(ex::just(42), ex::just(43, 44L), ex::just(), ex::just_or_error(di::Result<int>(45)));
    ASSERT_EQ(ex::sync_wait(di::move(s3)), di::make_tuple(42, 43, 44L, 45));

    auto executed = false;
    auto read_stop_token = ex::get_stop_token() | ex::then([&](di::concepts::StoppableToken auto stop_token) {
                               DI_ASSERT(stop_token.stop_requested());
                               executed = true;
                           });
    auto s4 = ex::when_all(ex::just_stopped(), read_stop_token);
    ASSERT_EQ(ex::sync_wait(di::move(s4)), di::Unexpected(di::BasicError::OperationCanceled));
    ASSERT(executed);

    executed = false;
    auto s5 = ex::when_all(ex::just_or_error(di::Result<int>(di::Unexpected(di::BasicError::InvalidArgument))),
                           read_stop_token);
    ASSERT_EQ(ex::sync_wait(di::move(s5)), di::Unexpected(di::BasicError::InvalidArgument));
    ASSERT(executed);
}

TEST(execution, meta)
TEST(execution, sync_wait)
TEST(execution, just)
TEST(execution, lazy)
TEST(execution, coroutine)
TEST(execution, then)
TEST(execution, inline_scheduler)
TEST(execution, let)
TEST(execution, transfer)
TEST(execution, as)
TEST(execution, with)
TEST(execution, any_sender)
TEST(execution, into_result)
TEST(execution, when_all)
}
