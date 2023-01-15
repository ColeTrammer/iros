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
    class AsyncFile {
    public:
        explicit AsyncFile(IoContext* parent, int fd) : m_parent(parent), m_fd(fd) {}

    private:
        struct ReadSender {
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
            friend auto tag_invoke(di::Tag<execution::connect>, ReadSender self, Receiver receiver) {
                return OperationState<Receiver> { self.parent, self.file_descriptor, self.buffer, self.offset, di::move(receiver) };
            }

            template<typename CPO>
            constexpr friend auto tag_invoke(execution::GetCompletionScheduler<CPO>, ReadSender const& self) {
                return self.parent->get_scheduler();
            }
        };

        struct WriteSender {
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
            friend auto tag_invoke(di::Tag<execution::connect>, WriteSender self, Receiver receiver) {
                return OperationState<Receiver> { self.parent, self.file_descriptor, self.buffer, self.offset, di::move(receiver) };
            }

            template<typename CPO>
            constexpr friend auto tag_invoke(execution::GetCompletionScheduler<CPO>, WriteSender const& self) {
                return self.parent->get_scheduler();
            }
        };

        friend auto tag_invoke(di::Tag<di::execution::async_read>, AsyncFile self, di::Span<di::Byte> buffer, di::Optional<u64> offset) {
            return ReadSender { self.m_parent, self.m_fd, buffer, offset };
        }

        friend auto tag_invoke(di::Tag<di::execution::async_write>, AsyncFile self, di::Span<di::Byte const> buffer,
                               di::Optional<u64> offset) {
            return WriteSender { self.m_parent, self.m_fd, buffer, offset };
        }

        friend auto tag_invoke(di::Tag<di::execution::async_destroy_in_place>, di::InPlaceType<AsyncFile>, AsyncFile& self) {
            auto file = SyncFile(SyncFile::Owned::Yes, self.m_fd);
            return di::execution::just();
        }

        IoContext* m_parent { nullptr };
        int m_fd { -1 };
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

        struct OpenSender {
        public:
            using CompletionSignatures = di::CompletionSignatures<di::SetValue(AsyncFile), di::SetError(di::Error)>;

            IoContext* parent;
            di::PathView path;
            OpenMode mode;
            u16 create_mode;

        private:
            template<typename Rec>
            struct OperationStateT {
                struct Type {
                    IoContext* parent;
                    di::PathView path;
                    OpenMode mode;
                    u16 create_mode;
                    [[no_unique_address]] Rec receiver;

                private:
                    friend void tag_invoke(di::Tag<execution::start>, Type& self) {
                        auto result = open_sync(self.path, self.mode, self.create_mode);
                        if (!result) {
                            execution::set_error(di::move(self.receiver), di::move(result).error());
                        } else {
                            execution::set_value(di::move(self.receiver), AsyncFile { self.parent, result->leak_file_descriptor() });
                        }
                    }
                };
            };

            template<di::ReceiverOf<CompletionSignatures> Receiver>
            using OperationState = di::meta::Type<OperationStateT<Receiver>>;

            template<di::ReceiverOf<CompletionSignatures> Receiver>
            friend auto tag_invoke(di::Tag<execution::connect>, OpenSender self, Receiver receiver) {
                return OperationState<Receiver> { self.parent, self.path, self.mode, self.create_mode, di::move(receiver) };
            }

            template<typename CPO>
            constexpr friend auto tag_invoke(execution::GetCompletionScheduler<CPO>, OpenSender const& self) {
                return self.parent->get_scheduler();
            }
        };

    public:
        IoContext* parent { nullptr };

    private:
        friend auto tag_invoke(di::Tag<execution::schedule>, Scheduler const& self) { return Sender { self.parent }; }

        friend auto tag_invoke(di::Tag<execution::async_open>, Scheduler const& self, di::PathView path, OpenMode mode,
                               u16 create_mode = 0666) {
            return OpenSender { self.parent, path, mode, create_mode };
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
                if (!state.queue.empty()) {
                    return di::make_tuple(di::address_of(*state.queue.pop()), false);
                }
                if (state.stopped) {
                    return di::make_tuple(nullptr, true);
                }
                return di::make_tuple(nullptr, false);
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