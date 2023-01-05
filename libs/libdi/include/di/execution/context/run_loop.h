#pragma once

#include <di/execution/concepts/receiver.h>
#include <di/execution/query/get_completion_scheduler.h>
#include <di/sync/dumb_spinlock.h>
#include <di/sync/synchronized.h>
#include <di/util/immovable.h>

namespace di::execution {
template<concepts::Lock Lock = sync::DumbSpinlock>
class RunLoop {
private:
    struct OperationStateBase : util::Immovable {
    public:
        virtual void execute() = 0;

        RunLoop* parent { nullptr };
        OperationStateBase* next { nullptr };
    };

    template<concepts::Receiver Receiver>
    struct OperationState : OperationStateBase {
    public:
        OperationState(RunLoop* parent, Receiver&& receiver) : RunLoop { parent, nullptr }, m_receiver(util::move(receiver)) {}

        virtual void execute() override {
            if (get_stop_token(m_receiver).stop_requested()) {
                set_stopped(util::move(m_receiver));
            } else {
                set_value(util::move(m_receiver));
            }
        }

    private:
        friend void tag_invoke(types::Tag<start>, OperationState& self) { self.parent->push_back(util::address_of(self)); }

        [[no_unique_address]] Receiver m_receiver;
    };

    struct Scheduler {
    private:
        struct Sender {
            using CompletionSignatures = types::CompletionSignatures<SetValue(), SetStopped()>;

            RunLoop* parent;

        private:
            template<concepts::Receiver Receiver>
            friend auto tag_invoke(types::Tag<connect>, Sender const& self, Receiver receiver) {}

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
        OperationStateBase* head { nullptr };
        OperationStateBase* tail { nullptr };
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
                if (state.stopped) {
                    return make_tuple(nullptr, true);
                }
                if (!state.head) {
                    return make_tuple(nullptr, false);
                }
                if (state.head == state.tail) {
                    auto* operation = state.head;
                    state.head = state.tail = nullptr;
                    return make_tuple(operation, false);
                }
                auto* operation = state.head;
                state.head = operation->next;
                return make_tuple(operation, false);
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
            if (!state.head) {
                state.head = state.tail = operation;
            } else {
                state.tail->next = operation;
                state.tail = operation;
            }
        });
    }

    sync::Synchronized<State, Lock> m_state;
};
}