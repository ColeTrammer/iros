#pragma once

#include <di/prelude.h>
#include <dius/sync_file.h>

namespace dius {
namespace execution = di::execution;

class IoContext {
private:
    struct OperationStateBase : di::IntrusiveForwardListElement<> {
    public:
        OperationStateBase(IoContext* parent_) : parent(parent_) {}

        virtual void execute() = 0;

        IoContext* parent { nullptr };
    };

    template<typename Receiver>
    struct OperationStateT {
        struct Type : OperationStateBase {
        public:
            Type(IoContext* parent, Receiver&& receiver) : OperationStateBase(parent), m_receiver(di::move(receiver)) {}

            virtual void execute() override {
                if (execution::get_stop_token(m_receiver).stop_requested()) {
                    execution::set_stopped(di::move(m_receiver));
                } else {
                    execution::set_value(di::move(m_receiver));
                }
            }

        private:
            void do_start() { this->parent->push_back(this); }

            friend void tag_invoke(di::Tag<execution::start>, Type& self) { self.do_start(); }

            [[no_unique_address]] Receiver m_receiver;
        };
    };

    template<typename Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    struct Scheduler;

public:
    struct AsyncReadSender {
    public:
        using CompletionSignatures = di::CompletionSignatures<di::SetValue(size_t), di::SetError(di::Error)>;

        IoContext* parent;
        int file_descriptor;
        di::Span<di::Byte> buffer;
        di::Optional<u64> offset;

    private:
        template<typename Rec>
        struct OperationStateT {
            struct Type {
                IoContext* parent;
                int file_descriptor;
                di::Span<di::Byte> buffer;
                di::Optional<u64> offset;
                [[no_unique_address]] Rec receiver;

            private:
                friend void tag_invoke(di::Tag<execution::start>, Type& self) {
                    auto sync_file = SyncFile(SyncFile::Owned::No, self.file_descriptor);
                    auto result = self.offset ? sync_file.read(self.offset.value(), self.buffer) : sync_file.read(self.buffer);
                    if (!result.has_value()) {
                        execution::set_error(di::move(self.receiver), di::move(result).error());
                    } else {
                        execution::set_value(di::move(self.receiver), di::move(result).value());
                    }
                }
            };
        };

        template<di::ReceiverOf<CompletionSignatures> Receiver>
        using OperationState = di::meta::Type<OperationStateT<Receiver>>;

        template<di::ReceiverOf<CompletionSignatures> Receiver>
        friend auto tag_invoke(di::Tag<execution::connect>, AsyncReadSender self, Receiver receiver) {
            return OperationState<Receiver> { self.parent, self.file_descriptor, self.buffer, self.offset, di::move(receiver) };
        }

        template<typename CPO>
        constexpr friend auto tag_invoke(execution::GetCompletionScheduler<CPO>, AsyncReadSender const& self) {
            return self.parent->get_scheduler();
        }
    };

    struct AsyncWriteSender {
    public:
        using CompletionSignatures = di::CompletionSignatures<di::SetValue(size_t), di::SetError(di::Error)>;

        IoContext* parent;
        int file_descriptor;
        di::Span<di::Byte const> buffer;
        di::Optional<u64> offset;

    private:
        template<typename Rec>
        struct OperationStateT {
            struct Type {
                IoContext* parent;
                int file_descriptor;
                di::Span<di::Byte const> buffer;
                di::Optional<u64> offset;
                [[no_unique_address]] Rec receiver;

            private:
                friend void tag_invoke(di::Tag<execution::start>, Type& self) {
                    auto sync_file = SyncFile(SyncFile::Owned::No, self.file_descriptor);
                    auto result = self.offset ? sync_file.write(self.offset.value(), self.buffer) : sync_file.write(self.buffer);
                    if (!result) {
                        execution::set_error(di::move(self.receiver), di::move(result).error());
                    } else {
                        execution::set_value(di::move(self.receiver), di::move(result).value());
                    }
                }
            };
        };

        template<di::ReceiverOf<CompletionSignatures> Receiver>
        using OperationState = di::meta::Type<OperationStateT<Receiver>>;

        template<di::ReceiverOf<CompletionSignatures> Receiver>
        friend auto tag_invoke(di::Tag<execution::connect>, AsyncWriteSender self, Receiver receiver) {
            return OperationState<Receiver> { self.parent, self.file_descriptor, self.buffer, self.offset, di::move(receiver) };
        }

        template<typename CPO>
        constexpr friend auto tag_invoke(execution::GetCompletionScheduler<CPO>, AsyncWriteSender const& self) {
            return self.parent->get_scheduler();
        }
    };

private:
    struct Scheduler {
    private:
        struct Sender {
            using CompletionSignatures = di::CompletionSignatures<di::SetValue(), di::SetStopped()>;

            IoContext* parent;

        private:
            template<di::ReceiverOf<CompletionSignatures> Receiver>
            friend auto tag_invoke(di::Tag<execution::connect>, Sender self, Receiver receiver) {
                return OperationState<Receiver> { self.parent, di::move(receiver) };
            }

            template<typename CPO>
            constexpr friend auto tag_invoke(execution::GetCompletionScheduler<CPO>, Sender const& self) {
                return self.parent->get_scheduler();
            }
        };

    public:
        IoContext* parent { nullptr };

    private:
        friend auto tag_invoke(di::Tag<execution::schedule>, Scheduler const& self) { return Sender { self.parent }; }

        friend auto tag_invoke(di::Tag<execution::async_read>, Scheduler const& self, int file_descriptor, di::Span<di::Byte> buffer,
                               di::Optional<u64> offset) {
            return AsyncReadSender { self.parent, file_descriptor, buffer, offset };
        }

        friend auto tag_invoke(di::Tag<execution::async_write>, Scheduler const& self, int file_descriptor, di::Span<di::Byte const> buffer,
                               di::Optional<u64> offset) {
            return AsyncWriteSender { self.parent, file_descriptor, buffer, offset };
        }

        constexpr friend bool operator==(Scheduler const&, Scheduler const&) = default;
    };

    struct State {
        di::Queue<OperationStateBase, di::IntrusiveForwardList<OperationStateBase>> queue;
        bool stopped { false };
    };

public:
    IoContext() {}
    IoContext(IoContext&&) = delete;

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
            auto [operation, is_stopped] = m_state.with_lock([](State& state) -> di::Tuple<OperationStateBase*, bool> {
                if (state.stopped) {
                    return di::make_tuple(nullptr, true);
                }
                if (state.queue.empty()) {
                    return di::make_tuple(nullptr, false);
                }
                return di::make_tuple(di::address_of(*state.queue.pop()), false);
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

    di::Synchronized<State, di::sync::DumbSpinlock> m_state;
};
}