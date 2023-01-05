#pragma once

#include <di/execution/context/run_loop.h>
#include <di/execution/query/get_delegatee_scheduler.h>
#include <di/execution/query/get_scheduler.h>
#include <di/function/pipeline.h>

namespace di::execution {
namespace sync_wait_ns {
    using Scheduler = decltype(util::declval<RunLoop<>&>().get_scheduler());

    struct Env {
    public:
        Scheduler scheduler;

    private:
        constexpr friend auto tag_invoke(types::Tag<get_scheduler>, Env const& self) { return self.scheduler; }
        constexpr friend auto tag_invoke(types::Tag<get_delegatee_scheduler>, Env const& self) { return self.scheduler; }
    };

    template<concepts::Sender<Env> Send>
    using ResultType = Result<meta::ValueTypesOf<Send, Env, meta::DecayedTuple, meta::TypeIdentity>>;

    template<concepts::Sender<Env> Send, concepts::Lock Lock = sync::DumbSpinlock>
    struct Receiver {
    public:
        explicit Receiver(ResultType<Send>* result, RunLoop<Lock>* run_loop)
            : m_result(result), m_run_loop(run_loop), m_env(run_loop->get_scheduler()) {}

    private:
        template<typename... Values>
        friend auto tag_invoke(types::Tag<set_value>, Receiver&& self, Values&&... values)
        requires(requires { self.m_result->emplace(util::forward<Values>(values)...); })
        {
            self.m_result->emplace(util::forward<Values>(values)...);
            self.m_run_loop->finish();
        }

        friend auto tag_invoke(types::Tag<set_error>, Receiver&& self, Error error) {
            // FIXME: handle other error types than the generic type-erased error.
            *self.m_result = Unexpected(util::move(error));
            self.m_run_loop->finish();
        }
        friend auto tag_invoke(types::Tag<set_stopped>, Receiver&& self) {
            *self.m_result = Unexpected(BasicError::Cancelled);
            self.m_run_loop->finish();
        }

        constexpr friend auto tag_invoke(types::Tag<get_env>, Receiver const& self) { return self.m_env; }

        ResultType<Send>* m_result;
        RunLoop<Lock>* m_run_loop;
        Env m_env;
    };

    template<typename T>
    struct Uninit {
        T value;
    };

    struct Function : function::pipeline::EnablePipeline {
        template<concepts::Sender<Env> Send>
        concepts::SameAs<ResultType<Send>> auto operator()(Send&& sender) const {
            if constexpr (requires {
                              function::tag_invoke(*this, get_completion_scheduler<SetValue>(sender), util::forward<Send>(sender));
                          }) {
                return function::tag_invoke(*this, get_completion_scheduler<SetValue>(sender), util::forward<Send>(sender));
            } else if constexpr (requires { function::tag_invoke(*this, util::forward<Send>(sender)); }) {
                return function::tag_invoke(*this, util::forward<Send>(sender));
            } else {
                auto run_loop = RunLoop<> {};
                auto value = Uninit<ResultType<Send>> {};

                auto operation =
                    connect(util::forward<Send>(sender), Receiver<Send>(util::address_of(value.value), util::address_of(run_loop)));
                start(operation);

                run_loop.run();

                return util::move(value.value);
            }
        }
    };
}

constexpr inline auto sync_wait = sync_wait_ns::Function {};
}