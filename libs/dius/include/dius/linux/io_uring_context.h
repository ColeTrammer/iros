#pragma once

#include <di/assert/prelude.h>
#include <di/container/algorithm/prelude.h>
#include <di/container/intrusive/prelude.h>
#include <di/container/queue/prelude.h>
#include <di/execution/prelude.h>
#include <di/function/prelude.h>
#include <di/util/prelude.h>
#include <dius/error.h>
#include <dius/linux/io_uring.h>

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
    friend OpenSender tag_invoke(di::Tag<di::execution::async_open>, IoUringScheduler const& self, di::Path path,
                                 OpenMode mode, u16 create_mode);
    friend OpenSender tag_invoke(di::Tag<di::execution::async_open>, IoUringScheduler const& self, di::Path path,
                                 OpenMode mode);

    constexpr friend bool operator==(IoUringScheduler const&, IoUringScheduler const&) = default;
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

    template<typename CPO>
    constexpr friend auto tag_invoke(di::execution::GetCompletionScheduler<CPO>, ReadSomeSender const& self) {
        return get_scheduler(self.parent);
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

    template<typename CPO>
    constexpr friend auto tag_invoke(di::execution::GetCompletionScheduler<CPO>, WriteSomeSender const& self) {
        return get_scheduler(self.parent);
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

    template<typename CPO>
    constexpr friend auto tag_invoke(di::execution::GetCompletionScheduler<CPO>, CloseSender const& self) {
        return get_scheduler(self.parent);
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

    template<typename CPO>
    constexpr friend auto tag_invoke(di::execution::GetCompletionScheduler<CPO>, ScheduleSender const& self) {
        return get_scheduler(self.parent);
    }
};

class AsyncFile {
public:
    explicit AsyncFile(IoUringContext* parent, int fd) : m_parent(parent), m_fd(fd) {}

private:
    friend auto tag_invoke(di::Tag<di::execution::async_read_some>, AsyncFile self, di::Span<di::Byte> buffer,
                           di::Optional<u64> offset) {
        return ReadSomeSender { self.m_parent, self.m_fd, buffer, offset };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_write_some>, AsyncFile self, di::Span<di::Byte const> buffer,
                           di::Optional<u64> offset) {
        return WriteSomeSender { self.m_parent, self.m_fd, buffer, offset };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_destroy_in_place>, di::InPlaceType<AsyncFile>,
                           AsyncFile& self) {
        return CloseSender { self.m_parent, self.m_fd };
    }

    IoUringContext* m_parent { nullptr };
    int m_fd { -1 };
};

struct OpenSender {
public:
    using is_sender = void;

    using CompletionSignatures =
        di::CompletionSignatures<di::SetValue(AsyncFile), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent;
    di::Path path;
    OpenMode mode;
    u16 create_mode;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, di::Path path, OpenMode mode, u16 create_mode, Rec receiver)
                : m_parent(parent)
                , m_path(di::move(path))
                , m_mode(mode)
                , m_create_mode(create_mode)
                , m_receiver(di::move(receiver)) {}

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    auto open_mode_flags = [&] {
                        switch (m_mode) {
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
                        sqe->addr = reinterpret_cast<u64>(m_path.c_str());
                        sqe->len = m_create_mode;
                        sqe->open_flags = open_mode_flags;
                    });
                }
            }

            void did_complete(io_uring::CQE const* cqe) override {
                if (cqe->res < 0) {
                    di::execution::set_error(di::move(m_receiver), di::Error(PosixError(-cqe->res)));
                } else {
                    di::execution::set_value(di::move(m_receiver), AsyncFile(m_parent, cqe->res));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                enqueue_operation(self.m_parent, di::addressof(self));
            }

            IoUringContext* m_parent;
            di::Path m_path;
            OpenMode m_mode;
            u16 m_create_mode;
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, OpenSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, di::move(self.path), self.mode, self.create_mode,
                                          di::move(receiver) };
    }

    template<typename CPO>
    constexpr friend auto tag_invoke(di::execution::GetCompletionScheduler<CPO>, OpenSender const& self) {
        return get_scheduler(self.parent);
    }
};

inline ScheduleSender tag_invoke(di::Tag<di::execution::schedule>, IoUringScheduler const& self) {
    return { self.parent };
}

inline OpenSender tag_invoke(di::Tag<di::execution::async_open>, IoUringScheduler const& self, di::Path path,
                             OpenMode mode, u16 create_mode) {
    return { self.parent, di::move(path), mode, create_mode };
}

inline OpenSender tag_invoke(di::Tag<di::execution::async_open>, IoUringScheduler const& self, di::Path path,
                             OpenMode mode) {
    return { self.parent, di::move(path), mode, 0666 };
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
    di::fill_n(reinterpret_cast<di::Byte*>(&sqe), sizeof(sqe), 0_b);
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
