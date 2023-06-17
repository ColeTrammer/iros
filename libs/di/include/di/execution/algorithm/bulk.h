#pragma once

#include <di/concepts/decay_constructible.h>
#include <di/concepts/expected.h>
#include <di/concepts/integral.h>
#include <di/concepts/movable_value.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/execution/concepts/receiver_of.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/meta/env_of.h>
#include <di/execution/meta/make_completion_signatures.h>
#include <di/execution/meta/value_types_of.h>
#include <di/execution/query/get_completion_signatures.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/receiver_adaptor.h>
#include <di/execution/receiver/set_error.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/curry_back.h>
#include <di/function/invoke.h>
#include <di/function/tag_invoke.h>
#include <di/meta/decay.h>
#include <di/meta/expected_error.h>
#include <di/meta/list/apply.h>
#include <di/meta/list/bind_front.h>
#include <di/meta/list/compose.h>
#include <di/meta/list/filter.h>
#include <di/meta/list/not.h>
#include <di/meta/list/same_as.h>
#include <di/meta/list/type.h>
#include <di/meta/remove_cvref.h>
#include <di/platform/compiler.h>
#include <di/util/addressof.h>
#include <di/util/move.h>

namespace di::execution {
namespace bulk_ns {
    template<typename Shape, typename Function, typename Rec>
    struct DataT {
        struct Type {
            Shape shape;
            [[no_unique_address]] Function function;
            [[no_unique_address]] Rec receiver;
        };
    };

    template<typename Shape, typename Function, typename Rec>
    using Data = meta::Type<DataT<meta::Decay<Shape>, meta::Decay<Function>, Rec>>;

    template<typename Shape, typename Function, typename Rec>
    struct ReceiverT {
        struct Type : ReceiverAdaptor<Type> {
        private:
            using Base = ReceiverAdaptor<Type>;
            friend Base;

        public:
            explicit Type(Data<Shape, Function, Rec>* data) : m_data(data) {}

            auto base() const& -> Rec const& { return m_data->receiver; }
            auto base() && -> Rec&& { return util::move(m_data->receiver); }

        private:
            template<typename... Args>
            requires(concepts::Invocable<Function&, Shape, Args&...>)
            void set_value(Args&&... args) && {
                for (auto i = Shape(); i != m_data->shape; ++i) {
                    if constexpr (concepts::Expected<meta::InvokeResult<Function&, Shape, Args&...>>) {
                        auto result = function::invoke(m_data->function, auto(i), args...);
                        if (!result) {
                            execution::set_error(util::move(*this).base(), util::move(result).error());
                            return;
                        }
                    } else {
                        (void) function::invoke(m_data->function, auto(i), args...);
                    }
                }
                execution::set_value(util::move(*this).base(), util::forward<Args>(args)...);
            }

            Data<Shape, Function, Rec>* m_data;
        };
    };

    template<typename Shape, typename Function, typename Rec>
    using Receiver = meta::Type<ReceiverT<meta::Decay<Shape>, meta::Decay<Function>, meta::Decay<Rec>>>;

    template<typename Send, typename Shape, typename Function, typename Rec>
    struct OperationStateT {
        struct Type {
        public:
            using Rc = Receiver<Shape, Function, Rec>;
            using Op = meta::ConnectResult<Send, Rc>;

            explicit Type(Send&& sender, Shape shape, Function&& function, Rec receiver)
                : m_data(shape, util::forward<Function>(function), util::move(receiver))
                , m_operation(connect(util::forward<Send>(sender), Rc(util::addressof(m_data)))) {}

        private:
            friend void tag_invoke(Tag<start>, Type& self) { start(self.m_operation); }

            [[no_unique_address]] Data<Shape, Function, Rec> m_data;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS Op m_operation;
        };
    };

    template<typename Send, typename Shape, typename Function, typename Rec>
    using OperationState = meta::Type<OperationStateT<Send, Shape, Function, Rec>>;

    template<typename Function, typename Shape>
    struct GetInvokeResult {
        template<typename... Args>
        using Invoke = meta::InvokeResult<Function&, Shape, Args&...>;
    };

    struct MakeErrorSigs {
        template<typename... Errors>
        using Invoke = CompletionSignatures<SetError(meta::ExpectedError<Errors>)...>;
    };

    template<typename Sender, typename Env, typename Shape, typename Function>
    using ErrorCompletions =
        meta::Apply<MakeErrorSigs,
                    meta::Filter<meta::ValueTypesOf<Sender, MakeEnv<Env>,
                                                    GetInvokeResult<Function, Shape>::template Invoke, meta::List>,
                                 meta::Not<meta::SameAs<void>>>>;

    template<typename Sender, typename Env, typename Shape, typename Function>
    using Sigs = meta::MakeCompletionSignatures<Sender, MakeEnv<Env>, ErrorCompletions<Sender, Env, Shape, Function>>;

    template<typename Send, typename Shape, typename Function>
    struct SenderT {
        struct Type {
            using is_sender = void;

            [[no_unique_address]] Send sender;
            [[no_unique_address]] Shape shape;
            [[no_unique_address]] Function function;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Env>
            requires(concepts::DecayConstructible<meta::Like<Self, Function>>)
            friend auto tag_invoke(Tag<get_completion_signatures>, Self&&, Env&&)
                -> Sigs<meta::Like<Self, Send>, Env, Shape, Function>;

            template<concepts::RemoveCVRefSameAs<Type> Self, typename Rec>
            requires(concepts::DecayConstructible<meta::Like<Self, Function>> &&
                     concepts::ReceiverOf<Rec, Sigs<meta::Like<Self, Send>, meta::EnvOf<Rec>, Shape, Function>>)
            friend auto tag_invoke(Tag<connect>, Self&& self, Rec receiver) {
                return OperationState<meta::Like<Self, Send>, Shape, meta::Like<Self, Function>, Rec>(
                    util::forward<Self>(self).sender, self.shape, util::forward<Self>(self).function,
                    util::move(receiver));
            }

            friend auto tag_invoke(Tag<get_env>, Type const& self) { return make_env(get_env(self.sender)); }
        };
    };

    template<typename Send, typename Shape, typename Function>
    using Sender = meta::Type<SenderT<meta::RemoveCVRef<Send>, meta::Decay<Shape>, meta::Decay<Function>>>;

    struct Function {
        template<concepts::Sender Send, concepts::Integral Shape, concepts::MovableValue Fun>
        constexpr auto operator()(Send&& sender, Shape shape, Fun&& function) const {
            if constexpr (requires {
                              tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                         util::forward<Send>(sender), shape, util::forward<Fun>(function));
                          }) {
                return tag_invoke(*this, get_completion_scheduler<SetValue>(get_env(sender)),
                                  util::forward<Send>(sender), shape, util::forward<Fun>(function));
            } else if constexpr (requires {
                                     tag_invoke(*this, util::forward<Send>(sender), shape,
                                                util::forward<Fun>(function));
                                 }) {
                return tag_invoke(*this, util::forward<Send>(sender), shape, util::forward<Fun>(function));
            } else {
                return Sender<Send, Shape, Fun> { util::forward<Send>(sender), shape, util::forward<Fun>(function) };
            }
        }
    };
}

/// @brief Bulk apply a function to a range of values.
///
/// @param sender The sender to send values from.
/// @param shape The number of values to send.
/// @param function The function to apply to each value.
///
/// @return A sender that applies the function to each value and sends the original values through unchanged.
///
/// This function is similar to container::for_each except that it is applied for every index in the specified shape.
/// The adapted sender will send the original values through unchanged, unless the function returns an error, in which
/// case the returned sender will send the error to the receiver.
///
/// On schedulers which support parallel execution, the scheduler should customize execution::bulk, which enables the
/// function to be applied in parallel.
///
/// The following example demonstrates how to sum over a vector of values in parallel, using a vector of intemediate
/// results to avoid contention.
/// @snippet{trimleft} tests/test_execution.cpp bulk
///
/// @warning The function must be thread-safe, if invoked on a parallel scheduler.
constexpr inline auto bulk = function::curry_back(bulk_ns::Function {}, c_<3zu>);
}
