#pragma once

#include <di/container/intrusive/prelude.h>
#include <di/container/queue/prelude.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/query/get_completion_scheduler.h>
#include <di/sync/dumb_spinlock.h>
#include <di/sync/synchronized.h>
#include <di/util/immovable.h>

namespace di::execution {
template<concepts::Lock Lock = sync::DumbSpinlock>
class RunLoop {
private:
    struct OperationStateBase : IntrusiveForwardListElement<> {
    public:
        OperationStateBase(RunLoop* parent_) : parent(parent_) {}

        virtual void execute() = 0;

        RunLoop* parent { nullptr };
    };

    template<typename Receiver>
    struct OperationStateT {
        struct Type : OperationStateBase {
        public:
            Type(RunLoop* parent, Receiver&& receiver) : OperationStateBase(parent), m_receiver(util::move(receiver)) {}

            virtual void execute() override {
                if (get_stop_token(m_receiver).stop_requested()) {
                    set_stopped(util::move(m_receiver));
                } else {
                    set_value(util::move(m_receiver));
                }
            }

        private:
            void do_start() { this->parent->push_back(this); }

            friend void tag_invoke(types::Tag<start>, Type& self) { self.do_start(); }

            [[no_unique_address]] Receiver m_receiver;
        };
    };

    template<typename Receiver>
    using OperationState = meta::Type<OperationStateT<Receiver>>;

    struct Scheduler {
    private:
        struct Sender {
            using CompletionSignatures = types::CompletionSignatures<SetValue(), SetStopped()>;

            RunLoop* parent;

        private:
            template<concepts::ReceiverOf<CompletionSignatures> Receiver>
            friend auto tag_invoke(types::Tag<connect>, Sender self, Receiver receiver) {
                return OperationState<Receiver> { self.parent, util::move(receiver) };
            }

            template<typename CPO>
            constexpr friend auto tag_invoke(GetCompletionScheduler<CPO>, Sender const& self) {
                return Scheduler { self.parent };
            }
        };

    public:
        RunLoop* parent { nullptr };

    private:
        friend auto tag_invoke(types::Tag<schedule>, Scheduler const& self) { return Sender { self.parent }; }

        constexpr friend bool operator==(Scheduler const&, Scheduler const&) = default;
    };

    struct State {
        Queue<OperationStateBase, IntrusiveForwardList<OperationStateBase>> queue;
        bool stopped { false };
    };

public:
    RunLoop() = default;
    RunLoop(RunLoop&&) = delete;

    Scheduler get_scheduler() { return Scheduler { this }; }

    void run() {
        while (auto* operation = pop_front()) {
            operation->execute();
        }
    }

    void finish() {
        m_state.with_lock([](State& state) {
            state.stopped = true;
        });
    }

private:
    OperationStateBase* pop_front() {
        // FIXME: block instead of busy polling the queue when it is empty.
        for (;;) {
            auto [operation, is_stopped] = m_state.with_lock([](State& state) -> Tuple<OperationStateBase*, bool> {
                // NOTE: even if a stop is requested, we must continue first empty the queue
                //       before returning stopping execution. Otherwise, the receiver contract
                //       will be violated (operation state will be destroyed without completion
                //       ever occuring).
                if (!state.queue.empty()) {
                    return make_tuple(util::address_of(*state.queue.pop()), false);
                }
                if (state.stopped) {
                    return make_tuple(nullptr, true);
                }
                return make_tuple(nullptr, false);
            });

            if (is_stopped) {
                return nullptr;
            } else if (operation) {
                return operation;
            }
        }
    }

    void push_back(OperationStateBase* operation) {
        m_state.with_lock([&](State& state) {
            state.queue.push(*operation);
        });
    }

    sync::Synchronized<State, Lock> m_state;
};
}