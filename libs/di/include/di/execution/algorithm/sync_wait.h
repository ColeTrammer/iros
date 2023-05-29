#pragma once

#include <di/execution/algorithm/into_variant.h>
#include <di/execution/concepts/sender_in.h>
#include <di/execution/context/run_loop.h>
#include <di/execution/meta/decayed_tuple.h>
#include <di/execution/query/get_delegatee_scheduler.h>
#include <di/execution/query/get_scheduler.h>
#include <di/function/curry.h>
#include <di/function/pipeline.h>
#include <di/meta/list/type.h>
#include <di/meta/type_constant.h>

namespace di::execution {
namespace sync_wait_ns {
    template<concepts::ExecutionContext Context>
    using Scheduler = meta::ExecutionContextScheduler<Context>;

    template<typename Context>
    struct EnvT {
        struct Type {
        public:
            Scheduler<Context> scheduler;

        private:
            constexpr friend auto tag_invoke(types::Tag<get_scheduler>, Type const& self) { return self.scheduler; }
            constexpr friend auto tag_invoke(types::Tag<get_delegatee_scheduler>, Type const& self) {
                return self.scheduler;
            }
        };
    };

    template<concepts::ExecutionContext Context>
    using Env = meta::Type<EnvT<Context>>;

    template<typename Result, typename Context>
    struct ReceiverT {
        struct Type {
        public:
            using is_receiver = void;

            explicit Type(Result* result, Context* context) : m_result(result), m_context(context) {}

        private:
            template<typename... Values>
            friend auto tag_invoke(SetValue, Type&& self, Values&&... values)
            requires(requires { util::declval<Result&>().emplace(util::forward<Values>(values)...); })
            {
                self.m_result->emplace(util::forward<Values>(values)...);
                self.m_context->finish();
            }

            friend void tag_invoke(SetError, Type&& self, Error error) {
                // FIXME: handle other error types than the generic type-erased error.
                *self.m_result = Unexpected(util::move(error));
                self.m_context->finish();
            }
            friend void tag_invoke(SetStopped, Type&& self) {
                *self.m_result = Unexpected(BasicError::OperationCanceled);
                self.m_context->finish();
            }

            constexpr friend auto tag_invoke(types::Tag<get_env>, Type const& self) {
                return Env<Context>(self.m_context->get_scheduler());
            }

            Result* m_result;
            Context* m_context;
        };
    };

    template<typename Result, concepts::ExecutionContext Context>
    using Receiver = meta::Type<ReceiverT<Result, Context>>;

    template<typename... Types>
    struct ResultTypeImplHelper : meta::TypeConstant<meta::DecayedTuple<Types...>> {};

    template<>
    struct ResultTypeImplHelper<> : meta::TypeConstant<void> {};

    template<typename T>
    struct ResultTypeImplHelper<T> : meta::TypeConstant<T> {};

    struct ResultTypeImpl {
        template<typename... Types>
        using Invoke = meta::Type<ResultTypeImplHelper<Types...>>;
    };

    template<typename... Types>
    struct ResultTypeConcatImplHelper {};

    template<>
    struct ResultTypeConcatImplHelper<> : meta::TypeConstant<void> {};

    template<typename T>
    struct ResultTypeConcatImplHelper<T> : meta::TypeConstant<T> {};

    struct ResultTypeConcatImpl {
        template<typename... Types>
        using Invoke = meta::Type<ResultTypeConcatImplHelper<Types...>>;
    };

    template<concepts::ExecutionContext Context, concepts::SenderIn<Env<Context>> Send>
    using ResultType = Result<
        meta::ValueTypesOf<Send, Env<Context>, ResultTypeImpl::template Invoke, ResultTypeConcatImpl::template Invoke>>;

    template<concepts::ExecutionContext Context, concepts::SenderIn<Env<Context>> Send>
    using WithVariantResultType = Result<into_variant_ns::IntoVariantType<Send, Env<Context>>>;

    template<typename T>
    struct Uninit {
        T value;
    };

    struct OnFunction {
        template<concepts::ExecutionContext Context, concepts::SenderIn<Env<Context>> Send>
        concepts::SameAs<ResultType<Context, Send>> auto operator()(Context& context, Send&& sender) const {
            if constexpr (requires {
                              function::tag_invoke(*this, context, get_completion_scheduler<SetValue>(get_env(sender)),
                                                   util::forward<Send>(sender));
                          }) {
                return function::tag_invoke(*this, context, get_completion_scheduler<SetValue>(get_env(sender)),
                                            util::forward<Send>(sender));
            } else if constexpr (requires { function::tag_invoke(*this, context, util::forward<Send>(sender)); }) {
                return function::tag_invoke(*this, context, util::forward<Send>(sender));
            } else {
                auto value = Uninit<ResultType<Context, Send>> {};

                auto operation = execution::connect(util::forward<Send>(sender),
                                                    Receiver<ResultType<Context, Send>, Context>(
                                                        util::addressof(value.value), util::addressof(context)));
                execution::start(operation);

                context.run();

                return util::move(value.value);
            }
        }
    };

    struct WithVariantOnFunction {
        template<concepts::ExecutionContext Context, concepts::SenderIn<Env<Context>> Send>
        concepts::SameAs<WithVariantResultType<Context, Send>> auto operator()(Context& context, Send&& sender) const {
            if constexpr (requires {
                              function::tag_invoke(*this, context, get_completion_scheduler<SetValue>(get_env(sender)),
                                                   util::forward<Send>(sender));
                          }) {
                return function::tag_invoke(*this, context, get_completion_scheduler<SetValue>(get_env(sender)),
                                            util::forward<Send>(sender));
            } else if constexpr (requires { function::tag_invoke(*this, context, util::forward<Send>(sender)); }) {
                return function::tag_invoke(*this, context, util::forward<Send>(sender));
            } else {
                return OnFunction {}(context, execution::into_variant(util::forward<Send>(sender)));
            }
        }
    };

    struct Function : function::pipeline::EnablePipeline {
        template<concepts::SenderIn<Env<RunLoop<>>> Send>
        concepts::SameAs<ResultType<RunLoop<>, Send>> auto operator()(Send&& sender) const {
            if constexpr (requires {
                              function::tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                                   util::forward<Send>(sender));
                          }) {
                return function::tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                            util::forward<Send>(sender));
            } else if constexpr (requires { function::tag_invoke(*this, util::forward<Send>(sender)); }) {
                return function::tag_invoke(*this, util::forward<Send>(sender));
            } else {
                auto run_loop = RunLoop<> {};
                return OnFunction {}(run_loop, util::forward<Send>(sender));
            }
        }
    };

    struct WithVariantFunction : function::pipeline::EnablePipeline {
        template<concepts::SenderIn<Env<RunLoop<>>> Send>
        concepts::SameAs<WithVariantResultType<RunLoop<>, Send>> auto operator()(Send&& sender) const {
            if constexpr (requires {
                              function::tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                                   util::forward<Send>(sender));
                          }) {
                return function::tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                            util::forward<Send>(sender));
            } else if constexpr (requires { function::tag_invoke(*this, util::forward<Send>(sender)); }) {
                return function::tag_invoke(*this, util::forward<Send>(sender));
            } else {
                return Function {}(execution::into_variant(util::forward<Send>(sender)));
            }
        }
    };
}

constexpr inline auto sync_wait = sync_wait_ns::Function {};
constexpr inline auto sync_wait_with_variant = sync_wait_ns::WithVariantFunction {};

constexpr inline auto sync_wait_on = function::curry(sync_wait_ns::OnFunction {}, meta::size_constant<2>);
constexpr inline auto sync_wait_with_variant_on =
    function::curry(sync_wait_ns::WithVariantOnFunction {}, meta::size_constant<2>);
}
