#pragma once

#ifdef DIUS_HAVE_LIBURING

#include <di/prelude.h>
#include <dius/error.h>
#include <dius/log.h>
#include <dius/sync_file.h>

#include <liburing.h>

namespace dius::linux_ {
struct IoUringContext;
struct IoUringContextImpl;
struct OperationStateBase;
struct IoUringScheduler;

template<di::concepts::Invocable<io_uring_sqe*> Fun>
static void enqueue_io_operation(IoUringContext*, OperationStateBase* op, Fun&& function);

void enqueue_operation(IoUringContext*, OperationStateBase*);

IoUringScheduler get_scheduler(IoUringContext*);

struct OperationStateBase : di::IntrusiveForwardListElement<> {
public:
    virtual void execute() = 0;
    virtual void did_complete(io_uring_cqe const*) {}
};

struct ReadSomeSender {
public:
    using CompletionSignatures = di::CompletionSignatures<di::SetValue(size_t), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent { nullptr };
    int file_descriptor { -1 };
    di::Span<di::Byte> buffer;
    di::Optional<u64> offset;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, int file_descriptor, di::Span<di::Byte> buffer, di::Optional<u64> offset, Rec receiver)
                : m_parent(parent)
                , m_file_descriptor(file_descriptor)
                , m_buffer(buffer)
                , m_offset(offset)
                , m_receiver(di::move(receiver)) {}

            virtual void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // Enqueue io_uring sqe with the read request.
                    enqueue_io_operation(m_parent, this, [&](auto* sqe) {
                        io_uring_prep_read(sqe, m_file_descriptor, m_buffer.data(), m_buffer.size(), m_offset.value_or((u64) -1));
                    });
                }
            }

            virtual void did_complete(io_uring_cqe const* cqe) override {
                if (cqe->res < 0) {
                    di::execution::set_error(di::move(m_receiver), di::Error(PosixError(-cqe->res)));
                } else {
                    di::execution::set_value(di::move(m_receiver), static_cast<size_t>(cqe->res));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) { enqueue_operation(self.m_parent, di::address_of(self)); }

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
        return OperationState<Receiver> { self.parent, self.file_descriptor, self.buffer, self.offset, di::move(receiver) };
    }

    template<typename CPO>
    constexpr friend auto tag_invoke(di::execution::GetCompletionScheduler<CPO>, ReadSomeSender const& self) {
        return get_scheduler(self.parent);
    }
};

struct WriteSomeSender {
public:
    using CompletionSignatures = di::CompletionSignatures<di::SetValue(size_t), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent { nullptr };
    int file_descriptor { -1 };
    di::Span<di::Byte const> buffer;
    di::Optional<u64> offset;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, int file_descriptor, di::Span<di::Byte const> buffer, di::Optional<u64> offset,
                          Rec receiver)
                : m_parent(parent)
                , m_file_descriptor(file_descriptor)
                , m_buffer(buffer)
                , m_offset(offset)
                , m_receiver(di::move(receiver)) {}

            virtual void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // Enqueue io_uring sqe with the read request.
                    enqueue_io_operation(m_parent, this, [&](auto* sqe) {
                        io_uring_prep_write(sqe, m_file_descriptor, m_buffer.data(), m_buffer.size(), m_offset.value_or((u64) -1));
                    });
                }
            }

            virtual void did_complete(io_uring_cqe const* cqe) override {
                if (cqe->res < 0) {
                    di::execution::set_error(di::move(m_receiver), di::Error(PosixError(-cqe->res)));
                } else {
                    di::execution::set_value(di::move(m_receiver), static_cast<size_t>(cqe->res));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) { enqueue_operation(self.m_parent, di::address_of(self)); }

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
        return OperationState<Receiver> { self.parent, self.file_descriptor, self.buffer, self.offset, di::move(receiver) };
    }

    template<typename CPO>
    constexpr friend auto tag_invoke(di::execution::GetCompletionScheduler<CPO>, WriteSomeSender const& self) {
        return get_scheduler(self.parent);
    }
};

struct ScheduleSender {
public:
    using CompletionSignatures = di::CompletionSignatures<di::SetValue(), di::SetStopped()>;

    IoUringContext* parent { nullptr };

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
        public:
            Type(IoUringContext* parent, Rec&& receiver) : m_parent(parent), m_receiver(di::move(receiver)) {}

            virtual void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    di::execution::set_value(di::move(m_receiver));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) { enqueue_operation(self.m_parent, di::address_of(self)); }

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

struct IoUringContext {
public:
    static di::Result<IoUringContext> create();

    IoUringContext(IoUringContext&&) = default;

    ~IoUringContext();

    IoUringScheduler get_scheduler();

    void run();
    void finish();

private:
    explicit IoUringContext(di::Box<IoUringContextImpl>);

public:
    di::Box<IoUringContextImpl> m_pimpl;
};

struct IoUringContextImpl {
    io_uring ring;
    di::Queue<OperationStateBase, di::IntrusiveForwardList<OperationStateBase>> queue;
    bool done { false };

    IoUringContextImpl() = default;

    IoUringContextImpl(IoUringContextImpl&&) = delete;

    void run();
    void finish() { done = true; }

    ~IoUringContextImpl() { io_uring_queue_exit(&ring); }
};

class AsyncFile {
public:
    explicit AsyncFile(IoUringContext* parent, int fd) : m_parent(parent), m_fd(fd) {}

private:
    friend auto tag_invoke(di::Tag<di::execution::async_read>, AsyncFile self, di::Span<di::Byte> buffer, di::Optional<u64> offset) {
        return ReadSomeSender { self.m_parent, self.m_fd, buffer, offset };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_write>, AsyncFile self, di::Span<di::Byte const> buffer, di::Optional<u64> offset) {
        return WriteSomeSender { self.m_parent, self.m_fd, buffer, offset };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_destroy_in_place>, di::InPlaceType<AsyncFile>, AsyncFile& self) {
        auto file = SyncFile(SyncFile::Owned::Yes, self.m_fd);
        return di::execution::just();
    }

    IoUringContext* m_parent { nullptr };
    int m_fd { -1 };
};

struct IoUringScheduler {
private:
    struct OpenSender {
    public:
        using CompletionSignatures = di::CompletionSignatures<di::SetValue(AsyncFile), di::SetError(di::Error)>;

        IoUringContext* parent;
        di::PathView path;
        OpenMode mode;
        u16 create_mode;

    private:
        template<typename Rec>
        struct OperationStateT {
            struct Type {
                IoUringContext* parent;
                di::PathView path;
                OpenMode mode;
                u16 create_mode;
                [[no_unique_address]] Rec receiver;

            private:
                friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                    auto result = open_sync(self.path, self.mode, self.create_mode);
                    if (!result) {
                        di::execution::set_error(di::move(self.receiver), di::move(result).error());
                    } else {
                        di::execution::set_value(di::move(self.receiver), AsyncFile { self.parent, result->leak_file_descriptor() });
                    }
                }
            };
        };

        template<di::ReceiverOf<CompletionSignatures> Receiver>
        using OperationState = di::meta::Type<OperationStateT<Receiver>>;

        template<di::ReceiverOf<CompletionSignatures> Receiver>
        friend auto tag_invoke(di::Tag<di::execution::connect>, OpenSender self, Receiver receiver) {
            return OperationState<Receiver> { self.parent, self.path, self.mode, self.create_mode, di::move(receiver) };
        }

        template<typename CPO>
        constexpr friend auto tag_invoke(di::execution::GetCompletionScheduler<CPO>, OpenSender const& self) {
            return get_scheduler(self.parent);
        }
    };

public:
    IoUringContext* parent { nullptr };

private:
    friend auto tag_invoke(di::Tag<di::execution::schedule>, IoUringScheduler const& self) { return ScheduleSender { self.parent }; }

    friend auto tag_invoke(di::Tag<di::execution::async_open>, IoUringScheduler const& self, di::PathView path, OpenMode mode,
                           u16 create_mode = 0666) {
        return OpenSender { self.parent, path, mode, create_mode };
    }

    constexpr friend bool operator==(IoUringScheduler const&, IoUringScheduler const&) = default;
};

inline di::Result<IoUringContext> IoUringContext::create() {
    auto pimpl = di::make_box<IoUringContextImpl>();

    int res = io_uring_queue_init(256, &pimpl->ring, 0);
    if (res < 0) {
        return di::Unexpected(PosixError(-res));
    }

    return IoUringContext(di::move(pimpl));
}

inline IoUringContext::IoUringContext(di::Box<IoUringContextImpl> pimpl) : m_pimpl(di::move(pimpl)) {}

inline IoUringContext::~IoUringContext() {}

inline void IoUringContextImpl::run() {
    for (;;) {
        // Reap any pending completions.
        io_uring_cqe* cqe;
        while (!io_uring_peek_cqe(&ring, &cqe)) {
            auto* as_operation = reinterpret_cast<OperationStateBase*>(cqe->user_data);
            as_operation->did_complete(cqe);
            io_uring_cqe_seen(&ring, cqe);
        }

        // Run locally available operations.
        while (!queue.empty()) {
            auto& item = *queue.pop();
            item.execute();
        }

        // If we're done, we're done.
        if (done) {
            break;
        }

        // Wait for some event to happen.
        io_uring_submit_and_wait(&ring, 1);
    }
}

template<di::concepts::Invocable<io_uring_sqe*> Fun>
static void enqueue_io_operation(IoUringContext* context, OperationStateBase* op, Fun&& function) {
    auto* sqe = io_uring_get_sqe(&context->m_pimpl->ring);
    ASSERT(sqe);
    di::invoke(di::move(function), sqe);
    sqe->user_data = reinterpret_cast<uintptr_t>(op);
}

inline void enqueue_operation(IoUringContext* context, OperationStateBase* op) {
    context->m_pimpl->queue.push(*op);
}

inline IoUringScheduler get_scheduler(IoUringContext* context) {
    return context->get_scheduler();
}

inline IoUringScheduler IoUringContext::get_scheduler() {
    return IoUringScheduler(this);
}

inline void IoUringContext::run() {
    return m_pimpl->run();
}

inline void IoUringContext::finish() {
    return m_pimpl->finish();
}
}
#endif