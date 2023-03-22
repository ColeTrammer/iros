// #include <di/prelude.h>
// #include <dius/prelude.h>
// #include <dius/test/prelude.h>

// static void meta() {
//     auto sender = di::execution::just(5);
//     auto sender2 = di::execution::just(5, 10);
//     auto sender3 = di::execution::just_error(5);
//     auto sender4 = di::execution::just_stopped();

//     static_assert(di::SameAs<di::meta::ValueTypesOf<decltype(sender)>, di::Variant<di::Tuple<int>>>);
//     static_assert(di::concepts::SenderOf<decltype(sender), di::NoEnv, int>);
//     static_assert(di::SameAs<di::meta::ValueTypesOf<decltype(sender2)>, di::Variant<di::Tuple<int, int>>>);
//     static_assert(di::SameAs<di::meta::ValueTypesOf<decltype(sender3)>, di::meta::detail::EmptyVariant>);
//     static_assert(di::SameAs<di::meta::ErrorTypesOf<decltype(sender3)>, di::Variant<int>>);
//     static_assert(!di::SendsStopped<decltype(sender3)>);
//     static_assert(di::SendsStopped<decltype(sender4)>);

//     static_assert(di::SameAs<di::meta::Unique<di::meta::List<int, short, int, int>>, di::meta::List<int, short>>);

//     using A = di::meta::MakeCompletionSignatures<
//         decltype(sender), di::types::NoEnv,
//         di::CompletionSignatures<di::SetValue(i64), di::SetStopped(), di::SetValue(i64)>>;
//     static_assert(
//         di::SameAs<A, di::types::CompletionSignatures<di::SetValue(i64), di::SetStopped(), di::SetValue(int)>>);

//     static_assert(di::SameAs<di::meta::AsList<di::Tuple<>>, di::meta::List<>>);

//     using S = decltype(di::declval<di::RunLoop<>>().get_scheduler());
//     using SS = decltype(di::execution::schedule(di::declval<S const&>()));
//     static_assert(di::Sender<SS>);
//     static_assert(di::TagInvocable<di::Tag<di::execution::get_completion_scheduler<di::SetValue>>, SS const&>);
//     static_assert(
//         di::SameAs<S, decltype(di::execution::get_completion_scheduler<di::SetValue>(di::declval<SS const&>()))>);
//     static_assert(di::Scheduler<S>);

//     static_assert(di::concepts::Awaitable<di::Lazy<i32>>);
//     static_assert(di::SameAs<di::CompletionSignatures<di::SetValue(i32), di::SetError(di::Error), di::SetStopped()>,
//                              decltype(di::execution::get_completion_signatures(di::declval<di::Lazy<i32>>()))>);

//     using R = di::meta::Type<di::execution::sync_wait_ns::Receiver<
//         di::execution::sync_wait_ns::ResultType<di::execution::RunLoop<>, di::Lazy<i32>>, di::execution::RunLoop<>>>;

//     static_assert(
//         di::SameAs<di::CompletionSignatures<di::SetValue(i32), di::SetError(di::Error), di::SetStopped()>,
//                    di::meta::Type<di::execution::connect_awaitable_ns::CompletionSignatures<di::Lazy<i32>, R>>>);

//     static_assert(di::SameAs<di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>,
//                              di::meta::Type<di::execution::connect_awaitable_ns::CompletionSignatures<di::Lazy<>,
//                              R>>>);

//     static_assert(di::concepts::Receiver<R>);
//     static_assert(di::concepts::ReceiverOf<
//                   R, di::CompletionSignatures<di::SetValue(i32), di::SetError(di::Error), di::SetStopped()>>);
//     static_assert(
//         di::concepts::ReceiverOf<R,
//                                  di::CompletionSignatures<di::SetValue(), di::SetError(di::Error),
//                                  di::SetStopped()>>);

//     static_assert(
//         di::SameAs<void,
//                    di::meta::ValueTypesOf<di::Lazy<>, di::types::NoEnv,
//                    di::meta::detail::SingleSenderValueTypeHelper,
//                                           di::meta::detail::SingleSenderValueTypeHelper>>);
//     static_assert(di::concepts::SingleSender<di::Lazy<i32>>);
//     static_assert(di::concepts::SingleSender<di::Lazy<>>);
// }

// static void sync_wait() {
//     namespace ex = di::execution;

//     ASSERT_EQ(ex::sync_wait(ex::just(42)), 42);

//     ASSERT(ex::sync_wait(ex::get_scheduler()));
//     ASSERT(ex::sync_wait(ex::get_stop_token()));
// }

// static void lazy() {
//     constexpr static auto t2 = [] -> di::Lazy<> {
//         co_return;
//     };

//     constexpr static auto task = [] -> di::Lazy<i32> {
//         co_await t2();
//         co_return 42;
//     };

//     ASSERT(di::sync_wait(t2()));
//     ASSERT_EQ(di::sync_wait(task()), 42);
// }

// static void coroutine() {
//     namespace ex = di::execution;

//     constexpr static auto task = [] -> di::Lazy<i32> {
//         auto x = co_await ex::just(42);
//         co_return x;
//     };
//     ASSERT_EQ(di::sync_wait(task()), 42);

//     constexpr static auto error = [] -> di::Lazy<i32> {
//         co_await ex::just_error(di::BasicError::Invalid);
//         co_return 56;
//     };
//     ASSERT_EQ(di::sync_wait(error()), di::Unexpected(di::BasicError::Invalid));

//     constexpr static auto stopped = [] -> di::Lazy<i32> {
//         co_await ex::just_stopped();
//         co_return 56;
//     };
//     ASSERT_EQ(di::sync_wait(stopped()), di::Unexpected(di::BasicError::Cancelled));
// }

// static void then() {
//     namespace ex = di::execution;

//     di::Sender auto work = ex::just(42) | ex::then([](int x) {
//                                return x * 2;
//                            });

//     using S = decltype(work);

//     using R =
//         ex::then_ns::Receiver<ex::sync_wait_ns::Receiver<di::Result<di::Tuple<int>>, ex::RunLoop<>>, di::Identity>;

//     static_assert(di::Receiver<R>);

//     static_assert(requires { ex::set_value(di::declval<R>(), 1); });
//     static_assert(di::ReceiverOf<R, di::CompletionSignatures<di::SetStopped()>>);
//     static_assert(di::ReceiverOf<R, di::CompletionSignatures<di::SetValue(i32)>>);
//     static_assert(di::SenderTo<S, R>);

//     di::Sender auto w2 = ex::just(42) | ex::then(di::into_void);

//     ASSERT_EQ(ex::sync_wait(di::move(work)), 84);
//     ASSERT(ex::sync_wait(di::move(w2)));
// }

// static void inline_scheduler() {
//     namespace ex = di::execution;

//     auto scheduler = di::InlineScheduler {};

//     auto work = ex::schedule(scheduler) | ex::then([] {
//                     return 42;
//                 });

//     auto w2 = ex::on(scheduler, ex::just(42));

//     ASSERT_EQ(ex::sync_wait(di::move(work)), 42);
//     ASSERT_EQ(ex::sync_wait(di::move(w2)), 42);

//     auto v = ex::on(scheduler, ex::just() | ex::let_value([] {
//                                    return ex::get_scheduler();
//                                }));
//     ASSERT_EQ(*ex::sync_wait(di::move(v)), di::make_tuple(scheduler));
// }

// static void let() {
//     namespace ex = di::execution;

//     auto w = ex::just(42) | ex::let_value(ex::just);
//     ASSERT_EQ(ex::sync_wait(di::move(w)), 42);

//     auto scheduler = di::InlineScheduler {};

//     auto v =
//         ex::schedule(scheduler) | ex::let_value(ex::get_scheduler) | ex::let_value([](di::Scheduler auto& scheduler)
//         {
//             return ex::schedule(scheduler) | ex::then([] {
//                        return 43;
//                    });
//         });
//     ASSERT_EQ(ex::sync_wait(di::move(v)), 43);

//     auto z = ex::just() | ex::then([] {
//                  return di::Result<long>(44);
//              }) |
//              ex::let_value([](long) {
//                  return ex::just(44);
//              });

//     ASSERT_EQ(ex::sync_wait(di::move(z)), 44);

//     auto y = ex::schedule(scheduler) | ex::let_value([] {
//                  return ex::just_error(di::BasicError::Invalid);
//              }) |
//              ex::let_error([](auto) {
//                  return ex::just(42);
//              }) |
//              ex::let_value([](auto) {
//                  return ex::just_error(di::BasicError::Invalid);
//              }) |
//              ex::let_error([](auto) {
//                  return ex::just_stopped();
//              }) |
//              ex::let_stopped([] {
//                  return ex::just(42);
//              });
//     ASSERT_EQ(ex::sync_wait(di::move(y)), 42);

//     auto a = ex::let_value_with(
//         [] {
//             return 42;
//         },
//         [](int& x) {
//             return ex::just(x);
//         });

//     ASSERT_EQ(ex::sync_wait(di::move(a)), 42);
// }

// static void transfer() {
//     namespace ex = di::execution;

//     auto scheduler = di::InlineScheduler {};

//     auto w = ex::transfer_just(scheduler, 42);

//     ASSERT_EQ(ex::sync_wait(di::move(w)), 42);
// }

// static void as() {
//     namespace ex = di::execution;

//     auto w = ex::just_stopped() | ex::stopped_as_optional;
//     ASSERT_EQ(ex::sync_wait(di::move(w)), di::make_tuple(di::nullopt));

//     auto v = ex::just_stopped() | ex::stopped_as_error(42) | ex::let_error([](int x) {
//                  DI_ASSERT_EQ(x, 42);
//                  return ex::just(x);
//              });
//     ASSERT_EQ(ex::sync_wait(di::move(v)), di::make_tuple(42));
// }

// struct AsyncI32 {
//     i32 value;

// private:
//     friend auto tag_invoke(di::Tag<di::execution::async_create_in_place>, di::InPlaceType<AsyncI32>, i32 value) {
//         return di::execution::just(AsyncI32 { value });
//     }

//     friend auto tag_invoke(di::Tag<di::execution::async_destroy_in_place>, di::InPlaceType<AsyncI32>, AsyncI32&) {
//         return di::execution::just();
//     }
// };

// static void with() {
//     namespace ex = di::execution;

//     auto w = ex::async_create<AsyncI32>(42) | ex::with([](AsyncI32& value) {
//                  return ex::just(value.value);
//              });

//     ASSERT_EQ(ex::sync_wait(di::move(w)), di::make_tuple(42));
// }

// TEST(execution, meta)
// TEST(execution, sync_wait)
// TEST(execution, lazy)
// TEST(execution, coroutine)
// TEST(execution, then)
// TEST(execution, inline_scheduler)
// TEST(execution, let)
// TEST(execution, transfer)
// TEST(execution, as)
// TEST(execution, with)
