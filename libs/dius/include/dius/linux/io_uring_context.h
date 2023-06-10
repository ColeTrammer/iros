#pragma once

#include <di/assert/prelude.h>
#include <di/container/algorithm/prelude.h>
#include <di/container/intrusive/prelude.h>
#include <di/container/queue/prelude.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/run.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/prelude.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/make_deferred.h>
#include <di/function/prelude.h>
#include <di/platform/compiler.h>
#include <di/util/prelude.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/variant/prelude.h>
#include <dius/error.h>
#include <dius/linux/io_uring.h>
#include <dius/sync_file.h>

namespace dius::linux {
struct IoUringContext;
struct IoUringContextImpl;
struct OperationStateBase;
struct IoUringScheduler;
struct ScheduleSender;
struct OpenSender;

template<di::concepts::Invocable<io_uring::SQE*> Fun>
static void enqueue_io_operation(IoUringContext*, OperationStateBase* op, Fun&& function);

inline void enqueue_operation(IoUringContext*, OperationStateBase*);

inline IoUringScheduler get_scheduler(IoUringContext*);

struct OperationStateBase : di::IntrusiveForwardListNode<> {
public:
    virtual void execute() = 0;
    virtual void did_complete(io_uring::CQE const*) {}
};

struct IoUringContext {
public:
    static di::Result<IoUringContext> create();

    IoUringContext(IoUringContext&&) = default;

    ~IoUringContext();

    IoUringScheduler get_scheduler();

    void run();
    void finish() { m_done = true; }

private:
    IoUringContext(io_uring::IoUringHandle handle) : m_handle(di::move(handle)) {};

public:
    io_uring::IoUringHandle m_handle;
    di::Queue<OperationStateBase, di::IntrusiveForwardList<OperationStateBase>> m_queue;
    bool m_done { false };
};

struct IoUringScheduler {
public:
    IoUringContext* parent { nullptr };

private:
    friend ScheduleSender tag_invoke(di::Tag<di::execution::schedule>, IoUringScheduler const& self);

    constexpr friend bool operator==(IoUringScheduler const&, IoUringScheduler const&) = default;
};

struct Env {
    IoUringContext* parent { nullptr };

    template<typename CPO>
    constexpr friend auto tag_invoke(di::execution::GetCompletionScheduler<CPO>, Env const& self) {
        return get_scheduler(self.parent);
    }
};

struct ReadSomeSender {
public:
    using is_sender = void;

    using CompletionSignatures =
        di::CompletionSignatures<di::SetValue(size_t), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent { nullptr };
    int file_descriptor { -1 };
    di::Span<di::Byte> buffer;
    di::Optional<u64> offset;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, int file_descriptor, di::Span<di::Byte> buffer,
                          di::Optional<u64> offset, Rec receiver)
                : m_parent(parent)
                , m_file_descriptor(file_descriptor)
                , m_buffer(buffer)
                , m_offset(offset)
                , m_receiver(di::move(receiver)) {}

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // Enqueue io_uring sqe with the read request.
                    enqueue_io_operation(m_parent, this, [&](auto* sqe) {
                        sqe->opcode = IORING_OP_READ;
                        sqe->fd = m_file_descriptor;
                        sqe->off = m_offset.value_or((u64) -1);
                        sqe->addr = reinterpret_cast<u64>(m_buffer.data());
                        sqe->len = m_buffer.size();
                    });
                }
            }

            void did_complete(io_uring::CQE const* cqe) override {
                if (cqe->res < 0) {
                    di::execution::set_error(di::move(m_receiver), di::Error(PosixError(-cqe->res)));
                } else {
                    di::execution::set_value(di::move(m_receiver), static_cast<size_t>(cqe->res));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                enqueue_operation(self.m_parent, di::addressof(self));
            }

            IoUringContext* m_parent;
            int m_file_descriptor;
            di::Span<di::Byte> m_buffer;
            di::Optional<u64> m_offset;
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, ReadSomeSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.file_descriptor, self.buffer, self.offset,
                                          di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, ReadSomeSender const& self) {
        return Env(self.parent);
    }
};

struct WriteSomeSender {
public:
    using is_sender = void;

    using CompletionSignatures =
        di::CompletionSignatures<di::SetValue(size_t), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent { nullptr };
    int file_descriptor { -1 };
    di::Span<di::Byte const> buffer;
    di::Optional<u64> offset;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, int file_descriptor, di::Span<di::Byte const> buffer,
                          di::Optional<u64> offset, Rec receiver)
                : m_parent(parent)
                , m_file_descriptor(file_descriptor)
                , m_buffer(buffer)
                , m_offset(offset)
                , m_receiver(di::move(receiver)) {}

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // Enqueue io_uring sqe with the write request.
                    enqueue_io_operation(m_parent, this, [&](auto* sqe) {
                        sqe->opcode = IORING_OP_WRITE;
                        sqe->fd = m_file_descriptor;
                        sqe->off = m_offset.value_or((u64) -1);
                        sqe->addr = reinterpret_cast<u64>(m_buffer.data());
                        sqe->len = m_buffer.size();
                    });
                }
            }

            void did_complete(io_uring::CQE const* cqe) override {
                if (cqe->res < 0) {
                    di::execution::set_error(di::move(m_receiver), di::Error(PosixError(-cqe->res)));
                } else {
                    di::execution::set_value(di::move(m_receiver), static_cast<size_t>(cqe->res));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                enqueue_operation(self.m_parent, di::addressof(self));
            }

            IoUringContext* m_parent;
            int m_file_descriptor;
            di::Span<di::Byte const> m_buffer;
            di::Optional<u64> m_offset;
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, WriteSomeSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.file_descriptor, self.buffer, self.offset,
                                          di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, WriteSomeSender const& self) {
        return Env(self.parent);
    }
};

struct CloseSender {
public:
    using is_sender = void;

    using CompletionSignatures = di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent { nullptr };
    int file_descriptor { -1 };

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, int file_descriptor, Rec receiver)
                : m_parent(parent), m_file_descriptor(file_descriptor), m_receiver(di::move(receiver)) {}

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // Enqueue io_uring sqe with the close request.
                    enqueue_io_operation(m_parent, this, [&](auto* sqe) {
                        sqe->opcode = IORING_OP_CLOSE;
                        sqe->fd = m_file_descriptor;
                    });
                }
            }

            void did_complete(io_uring::CQE const* cqe) override {
                if (cqe->res < 0) {
                    di::execution::set_error(di::move(m_receiver), di::Error(PosixError(-cqe->res)));
                } else {
                    di::execution::set_value(di::move(m_receiver));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                enqueue_operation(self.m_parent, di::addressof(self));
            }

            IoUringContext* m_parent;
            int m_file_descriptor;
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, CloseSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.file_descriptor, di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, CloseSender const& self) {
        return Env(self.parent);
    }
};

struct ScheduleSender {
public:
    using is_sender = void;

    using CompletionSignatures = di::CompletionSignatures<di::SetValue(), di::SetStopped()>;

    IoUringContext* parent { nullptr };

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
        public:
            Type(IoUringContext* parent, Rec&& receiver) : m_parent(parent), m_receiver(di::move(receiver)) {}

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    di::execution::set_value(di::move(m_receiver));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                enqueue_operation(self.m_parent, di::addressof(self));
            }

            IoUringContext* m_parent { nullptr };
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::Receiver Rec>
    using OperationState = di::meta::Type<OperationStateT<Rec>>;

    template<di::ReceiverOf<CompletionSignatures> Rec>
    friend auto tag_invoke(di::Tag<di::execution::connect>, ScheduleSender self, Rec receiver) {
        return OperationState<Rec> { self.parent, di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, ScheduleSender const& self) {
        return Env { self.parent };
    }
};

class AsyncFile : di::Immovable {
public:
    explicit AsyncFile(IoUringContext* parent, di::Path path, OpenMode mode, u16 create_mode)
        : m_parent(parent), m_path(di::move(path)), m_mode(mode), m_create_mode(create_mode) {}

    IoUringContext* parent() const { return m_parent; }
    di::Path const& path() const { return m_path; }
    OpenMode mode() const { return m_mode; }
    u16 create_mode() const { return m_create_mode; }

    int fd() const { return m_fd; }
    void set_fd(int fd) { m_fd = fd; }

private:
    friend auto tag_invoke(di::Tag<di::execution::async_read_some>, AsyncFile& self, di::Span<di::Byte> buffer,
                           di::Optional<u64> offset) {
        return ReadSomeSender { self.m_parent, self.m_fd, buffer, offset };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_write_some>, AsyncFile& self, di::Span<di::Byte const> buffer,
                           di::Optional<u64> offset) {
        return WriteSomeSender { self.m_parent, self.m_fd, buffer, offset };
    }

    IoUringContext* m_parent;
    di::Path m_path;
    OpenMode m_mode;
    u16 m_create_mode;
    int m_fd { -1 };
};

struct OpenSender {
public:
    using is_sender = void;

    using CompletionSignatures = di::CompletionSignatures<di::SetValue(di::ReferenceWrapper<AsyncFile>),
                                                          di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent;
    di::ReferenceWrapper<AsyncFile> file;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, di::ReferenceWrapper<AsyncFile> file, Rec&& receiver)
                : m_parent(parent), m_file(file), m_receiver(di::move(receiver)) {}

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    auto open_mode_flags = [&] {
                        switch (m_file.get().mode()) {
                            case OpenMode::Readonly:
                                return O_RDONLY;
                            case OpenMode::WriteNew:
                                return O_WRONLY | O_EXCL | O_CREAT;
                            case OpenMode::WriteClobber:
                                return O_WRONLY | O_TRUNC | O_CREAT;
                            case OpenMode::ReadWrite:
                                return O_RDWR;
                            case OpenMode::AppendOnly:
                                return O_WRONLY | O_APPEND | O_CREAT;
                            default:
                                di::unreachable();
                        }
                    }();

                    // Enqueue io_uring sqe with the open request.
                    enqueue_io_operation(m_parent, this, [&](auto* sqe) {
                        sqe->opcode = IORING_OP_OPENAT;
                        sqe->fd = AT_FDCWD;
                        sqe->addr = reinterpret_cast<u64>(m_file.get().path().c_str());
                        sqe->len = m_file.get().create_mode();
                        sqe->open_flags = open_mode_flags;
                    });
                }
            }

            void did_complete(io_uring::CQE const* cqe) override {
                if (cqe->res < 0) {
                    di::execution::set_error(di::move(m_receiver), di::Error(PosixError(-cqe->res)));
                } else {
                    m_file.get().set_fd(cqe->res);
                    di::execution::set_value(di::move(m_receiver), di::ref(m_file));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                enqueue_operation(self.m_parent, di::addressof(self));
            }

            IoUringContext* m_parent;
            di::ReferenceWrapper<AsyncFile> m_file;
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, OpenSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.file, di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, OpenSender const& self) {
        return Env(self.parent);
    }
};

struct RunSender {
public:
    using is_sender = di::SequenceTag;

    using CompletionSignatures = di::CompletionSignatures<di::SetValue(di::ReferenceWrapper<AsyncFile>),
                                                          di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent;
    di::ReferenceWrapper<AsyncFile> file;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : di::Immovable {
            struct Rec1 {
                using is_receiver = void;

                di::Function<void()> complete;

                friend auto tag_invoke(di::Tag<di::execution::set_value>, Rec1&& self) { self.complete(); }
                friend auto tag_invoke(di::Tag<di::execution::set_stopped>, Rec1&& self) { self.complete(); }
            };

            struct Rec2 : di::ReceiverAdaptor<Rec2> {
            private:
                using Base = di::ReceiverAdaptor<Rec2>;
                friend Base;

            public:
                explicit Rec2(Rec* receiver) : m_receiver(receiver) {}

                Rec const& base() const& { return *m_receiver; }
                Rec&& base() && { return di::move(*m_receiver); }

            private:
                Rec* m_receiver;
            };

            explicit Type(IoUringContext* parent, di::ReferenceWrapper<AsyncFile> file, Rec&& receiver)
                : m_parent(parent), m_file(file), m_receiver(di::move(receiver)) {}

        private:
            using NextSender = di::meta::NextSenderOf<Rec, OpenSender>;
            using Op1 = di::meta::ConnectResult<NextSender, Rec1>;
            using Op2 = di::meta::ConnectResult<CloseSender, Rec2>;

            void finish_phase1() {
                auto& op = m_op.template emplace<2>(di::DeferConstruct([&] {
                    return di::execution::connect(CloseSender(m_parent, m_file.get().fd()),
                                                  Rec2(di::addressof(m_receiver)));
                }));
                di::execution::start(op);
            }

            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                auto& op = self.m_op.template emplace<1>(di::DeferConstruct([&] {
                    return di::execution::connect(
                        di::execution::set_next(self.m_receiver, OpenSender(self.m_parent, self.m_file)), Rec1([&self] {
                            return self.finish_phase1();
                        }));
                }));
                di::execution::start(op);
            }

            IoUringContext* m_parent;
            di::ReferenceWrapper<AsyncFile> m_file;
            [[no_unique_address]] Rec m_receiver;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS di::Variant<di::Void, Op1, Op2> m_op;
        };
    };

    template<typename Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<
        di::ReceiverOf<di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::subscribe>, RunSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.file, di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, RunSender const& self) {
        return di::execution::make_env(Env(self.parent),
                                       di::execution::with(di::execution::get_sequence_cardinality, di::c_<1zu>));
    }
};

inline auto tag_invoke(di::Tag<di::execution::run>, AsyncFile& self) {
    return RunSender { self.parent(), self };
}

inline ScheduleSender tag_invoke(di::Tag<di::execution::schedule>, IoUringScheduler const& self) {
    return { self.parent };
}

inline auto tag_invoke(di::Tag<di::execution::async_open>, IoUringScheduler const& self, di::Path path, OpenMode mode,
                       u16 create_mode) {
    return di::make_deferred<AsyncFile>(self.parent, di::move(path), mode, create_mode);
}

inline auto tag_invoke(di::Tag<di::execution::async_open>, IoUringScheduler const& self, di::Path path, OpenMode mode) {
    return di::make_deferred<AsyncFile>(self.parent, di::move(path), mode, 0666);
}

inline di::Result<IoUringContext> IoUringContext::create() {
    return IoUringContext(TRY(io_uring::IoUringHandle::create()));
}

inline IoUringContext::~IoUringContext() = default;

inline void IoUringContext::run() {
    for (;;) {
        // Reap any pending completions.
        while (auto cqe = m_handle.get_next_cqe()) {
            auto* as_operation = reinterpret_cast<OperationStateBase*>(cqe->user_data);
            as_operation->did_complete(cqe.data());
        }

        // Run locally available operations.
        while (!m_queue.empty()) {
            auto& item = *m_queue.pop();
            item.execute();
        }

        // If we're done, we're done.
        if (m_done) {
            break;
        }

        // Wait for some event to happen.
        (void) m_handle.submit_and_wait();
    }
}

template<di::concepts::Invocable<io_uring::SQE*> Fun>
inline void enqueue_io_operation(IoUringContext* context, OperationStateBase* op, Fun&& function) {
    auto sqe = context->m_handle.get_next_sqe();
    ASSERT(sqe);
    di::fill_n(reinterpret_cast<di::Byte*>(sqe.data()), sizeof(sqe), 0_b);
    di::invoke(di::forward<Fun>(function), sqe.data());
    sqe->user_data = reinterpret_cast<uintptr_t>(op);
}

inline void enqueue_operation(IoUringContext* context, OperationStateBase* op) {
    context->m_queue.push(*op);
}

inline IoUringScheduler get_scheduler(IoUringContext* context) {
    return context->get_scheduler();
}

inline IoUringScheduler IoUringContext::get_scheduler() {
    return IoUringScheduler(this);
}
}
