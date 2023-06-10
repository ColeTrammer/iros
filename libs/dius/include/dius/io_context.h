#pragma once

#include <di/container/intrusive/prelude.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/concepts/receiver.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/get_env.h>
#include <di/execution/interface/run.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/prelude.h>
#include <di/execution/query/get_sequence_cardinality.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/set_value.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/function/make_deferred.h>
#include <di/function/prelude.h>
#include <di/platform/compiler.h>
#include <di/sync/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/optional/prelude.h>
#include <dius/config.h>
#include <dius/sync_file.h>

#ifdef DIUS_PLATFORM_LINUX
#include <dius/linux/io_uring_context.h>
#endif

namespace dius {
#ifdef DIUS_PLATFORM_LINUX
using IoContext = linux::IoUringContext;
#else
namespace execution = di::execution;

class IoContext {
private:
    struct OperationStateBase : di::IntrusiveForwardListNode<> {
    public:
        OperationStateBase(IoContext* parent_) : parent(parent_) {}

        virtual void execute() = 0;

        IoContext* parent { nullptr };
    };

    struct Env {
        IoContext* parent;

        template<typename CPO>
        constexpr friend auto tag_invoke(execution::GetCompletionScheduler<CPO>, Env const& self) {
            return self.parent->get_scheduler();
        }
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

    struct Scheduler {
    private:
        struct Sender {
            using is_sender = void;

            using CompletionSignatures = di::CompletionSignatures<di::SetValue(), di::SetStopped()>;

            IoContext* parent;

        private:
            template<di::ReceiverOf<CompletionSignatures> Receiver>
            friend auto tag_invoke(di::Tag<execution::connect>, Sender self, Receiver receiver) {
                return OperationState<Receiver> { self.parent, di::move(receiver) };
            }

            constexpr friend auto tag_invoke(di::Tag<execution::get_env>, Sender const& self) {
                return Env { self.parent };
            }
        };

        struct AsyncFile : di::Immovable {
        public:
            explicit AsyncFile(IoContext* parent, di::PathView path, OpenMode mode, u16 create_mode)
                : m_parent(parent), m_path(path), m_mode(mode), m_create_mode(create_mode) {}

        private:
            struct ReadSomeSender {
            public:
                using is_sender = void;

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
                            auto result = self.offset ? sync_file.read_some(self.offset.value(), self.buffer)
                                                      : sync_file.read_some(self.buffer);
                            if (!result.has_value()) {
                                execution::set_error(di::move(self.receiver), di::Error(di::move(result).error()));
                            } else {
                                execution::set_value(di::move(self.receiver), di::move(result).value());
                            }
                        }
                    };
                };

                template<di::ReceiverOf<CompletionSignatures> Receiver>
                using OperationState = di::meta::Type<OperationStateT<Receiver>>;

                template<di::ReceiverOf<CompletionSignatures> Receiver>
                friend auto tag_invoke(di::Tag<execution::connect>, ReadSomeSender self, Receiver receiver) {
                    return OperationState<Receiver> { self.parent, self.file_descriptor, self.buffer, self.offset,
                                                      di::move(receiver) };
                }

                constexpr friend auto tag_invoke(di::Tag<execution::get_env>, ReadSomeSender const& self) {
                    return Env { self.parent };
                }
            };

            struct WriteSomeSender {
            public:
                using is_sender = void;

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
                            auto result = self.offset ? sync_file.write_some(self.offset.value(), self.buffer)
                                                      : sync_file.write_some(self.buffer);
                            if (!result) {
                                execution::set_error(di::move(self.receiver), di::Error(di::move(result).error()));
                            } else {
                                execution::set_value(di::move(self.receiver), di::move(result).value());
                            }
                        }
                    };
                };

                template<di::ReceiverOf<CompletionSignatures> Receiver>
                using OperationState = di::meta::Type<OperationStateT<Receiver>>;

                template<di::ReceiverOf<CompletionSignatures> Receiver>
                friend auto tag_invoke(di::Tag<execution::connect>, WriteSomeSender self, Receiver receiver) {
                    return OperationState<Receiver> { self.parent, self.file_descriptor, self.buffer, self.offset,
                                                      di::move(receiver) };
                }

                constexpr friend auto tag_invoke(di::Tag<execution::get_env>, WriteSomeSender const& self) {
                    return Env { self.parent };
                }
            };

            struct OpenSender {
            public:
                using is_sender = void;

                using CompletionSignatures =
                    di::CompletionSignatures<di::SetValue(di::ReferenceWrapper<AsyncFile>), di::SetError(di::Error)>;

                IoContext* parent;
                di::ReferenceWrapper<AsyncFile> file;

            private:
                template<typename Rec>
                struct OperationStateT {
                    struct Type {
                        IoContext* parent;
                        di::ReferenceWrapper<AsyncFile> file;
                        [[no_unique_address]] Rec receiver;

                    private:
                        friend void tag_invoke(di::Tag<execution::start>, Type& self) {
                            auto result = open_sync(self.file.get().m_path, self.file.get().m_mode,
                                                    self.file.get().m_create_mode);
                            if (!result) {
                                execution::set_error(di::move(self.receiver), di::Error(di::move(result).error()));
                            } else {
                                self.file.get().m_file = di::move(result).value();
                                execution::set_value(di::move(self.receiver), self.file);
                            }
                        }
                    };
                };

                template<di::ReceiverOf<CompletionSignatures> Receiver>
                using OperationState = di::meta::Type<OperationStateT<Receiver>>;

                template<di::ReceiverOf<CompletionSignatures> Receiver>
                friend auto tag_invoke(di::Tag<execution::connect>, OpenSender self, Receiver receiver) {
                    return OperationState<Receiver> { self.parent, self.file, di::move(receiver) };
                }

                constexpr friend auto tag_invoke(di::Tag<execution::get_env>, OpenSender const& self) {
                    return Env { self.parent };
                }
            };

            struct RunSender {
            public:
                using is_sender = di::SequenceTag;

                using CompletionSignatures =
                    di::CompletionSignatures<di::SetValue(di::ReferenceWrapper<AsyncFile>), di::SetError(di::Error)>;

                IoContext* parent;
                di::ReferenceWrapper<AsyncFile> file;

            private:
                template<typename Rec>
                struct OperationStateT {
                    struct Type {
                        struct Receiver {
                            using is_receiver = void;

                            Type* parent;

                            friend void tag_invoke(di::Tag<execution::set_value>, Receiver&& self) {
                                self.parent->close();
                            }
                            friend void tag_invoke(di::Tag<execution::set_stopped>, Receiver&& self) {
                                self.parent->close();
                            }
                        };

                        using Next = di::meta::NextSenderOf<Rec, OpenSender>;
                        using Op = di::meta::ConnectResult<Next, Receiver>;

                        IoContext* parent;
                        di::ReferenceWrapper<AsyncFile> file;
                        [[no_unique_address]] Rec receiver;
                        DI_IMMOVABLE_NO_UNIQUE_ADDRESS di::Optional<Op> op;

                        void close() {
                            auto result = file.get().m_file.close();
                            if (!result) {
                                execution::set_error(di::move(receiver), di::Error(di::move(result).error()));
                            } else {
                                execution::set_value(di::move(receiver));
                            }
                        }

                    private:
                        friend void tag_invoke(di::Tag<execution::start>, Type& self) {
                            self.op.emplace(di::DeferConstruct([&] {
                                return execution::connect(
                                    execution::set_next(self.receiver, OpenSender(self.parent, self.file)),
                                    Receiver(&self));
                            }));
                            execution::start(*self.op);
                        }
                    };
                };

                template<typename Receiver>
                using OperationState = di::meta::Type<OperationStateT<Receiver>>;

                template<di::concepts::Receiver Receiver>
                friend auto tag_invoke(di::Tag<execution::subscribe>, RunSender self, Receiver receiver) {
                    return OperationState<Receiver> { self.parent, self.file, di::move(receiver), di::nullopt };
                }

                friend auto tag_invoke(di::Tag<execution::get_env>, RunSender const& self) {
                    return execution::make_env(Env(self.parent),
                                               execution::with(execution::get_sequence_cardinality, di::c_<1zu>));
                }
            };

            friend auto tag_invoke(di::Tag<di::execution::async_read_some>, AsyncFile& self, di::Span<di::Byte> buffer,
                                   di::Optional<u64> offset) {
                return ReadSomeSender { self.m_parent, self.m_file.file_descriptor(), buffer, offset };
            }

            friend auto tag_invoke(di::Tag<di::execution::async_write_some>, AsyncFile& self,
                                   di::Span<di::Byte const> buffer, di::Optional<u64> offset) {
                return WriteSomeSender { self.m_parent, self.m_file.file_descriptor(), buffer, offset };
            }

            friend auto tag_invoke(di::Tag<di::execution::run>, AsyncFile& self) {
                return RunSender(self.m_parent, di::ref(self));
            }

            IoContext* m_parent;
            di::PathView m_path;
            OpenMode m_mode;
            u16 m_create_mode;
            SyncFile m_file;
        };

    public:
        IoContext* parent { nullptr };

    private:
        friend auto tag_invoke(di::Tag<execution::schedule>, Scheduler const& self) { return Sender { self.parent }; }

        friend auto tag_invoke(di::Tag<execution::async_open>, Scheduler const& self, di::PathView path, OpenMode mode,
                               u16 create_mode = 0666) {
            return di::make_deferred<AsyncFile>(self.parent, path, mode, create_mode);
        }

        constexpr friend bool operator==(Scheduler const&, Scheduler const&) = default;
    };

    struct State {
        State() {};

        di::Queue<OperationStateBase, di::IntrusiveForwardList<OperationStateBase>> queue;
        bool stopped { false };
    };

    di::Synchronized<State>& state() { return m_state.value(); }

public:
    static di::Result<IoContext> create() { return IoContext {}; }

    IoContext(IoContext&&) = default;

    Scheduler get_scheduler() { return Scheduler { this }; }

    void run() {
        while (auto* operation = pop_front()) {
            operation->execute();
        }
    }

    void finish() {
        state().with_lock([](State& state) {
            state.stopped = true;
        });
    }

private:
    IoContext() {}

    OperationStateBase* pop_front() {
        // FIXME: block instead of busy polling the queue when it is empty.
        for (;;) {
            auto [operation, is_stopped] = state().with_lock([](State& state) -> di::Tuple<OperationStateBase*, bool> {
                if (!state.queue.empty()) {
                    return di::make_tuple(di::addressof(*state.queue.pop()), false);
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
        state().with_lock([&](State& state) {
            state.queue.push(*operation);
        });
    }

    di::MovableBox<di::Synchronized<State>> m_state;
};
#endif
}
