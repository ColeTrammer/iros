#pragma once

#include <di/assert/prelude.h>
#include <di/container/algorithm/copy.h>
#include <di/container/algorithm/prelude.h>
#include <di/container/intrusive/prelude.h>
#include <di/container/queue/prelude.h>
#include <di/execution/interface/connect.h>
#include <di/execution/interface/run.h>
#include <di/execution/io/async_net.h>
#include <di/execution/meta/connect_result.h>
#include <di/execution/prelude.h>
#include <di/execution/query/make_env.h>
#include <di/execution/receiver/prelude.h>
#include <di/execution/sequence/sequence_sender.h>
#include <di/execution/types/completion_signuatures.h>
#include <di/function/make_deferred.h>
#include <di/function/prelude.h>
#include <di/meta/operations.h>
#include <di/platform/compiler.h>
#include <di/util/addressof.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/variant/prelude.h>
#include <dius/error.h>
#include <dius/linux/error.h>
#include <dius/linux/io_uring.h>
#include <dius/linux/system_call.h>
#include <dius/net/address.h>
#include <dius/net/socket.h>
#include <dius/sync_file.h>

#ifdef DIUS_USE_RUNTIME
#include <linux/socket.h>
#include <linux/un.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#endif

namespace dius::linux {
struct IoUringContext;
struct IoUringContextImpl;
struct OperationStateBase;
struct IoUringScheduler;
struct ScheduleSender;
struct OpenSender;
struct AcceptSender;
struct MakeSocketSender;

template<di::concepts::Invocable<io_uring::SQE*> Fun>
static void enqueue_io_operation(IoUringContext*, OperationStateBase* op, Fun&& function);

inline void enqueue_operation(IoUringContext*, OperationStateBase*);

inline auto get_scheduler(IoUringContext*) -> IoUringScheduler;

struct OperationStateBase : di::IntrusiveForwardListNode<> {
public:
    virtual void execute() = 0;
    virtual void did_complete(io_uring::CQE const*) {}
};

struct IoUringContext {
public:
    static auto create() -> di::Result<IoUringContext>;

    IoUringContext(IoUringContext&& other) : m_handle(di::move(other.m_handle)) {}

    ~IoUringContext();

    auto get_scheduler() -> IoUringScheduler;

    void run();
    void finish() { m_done.store(true); }

private:
    IoUringContext(io_uring::IoUringHandle handle) : m_handle(di::move(handle)) {};

public:
    io_uring::IoUringHandle m_handle;
    di::Queue<OperationStateBase, di::IntrusiveForwardList<OperationStateBase>> m_queue;
    di::Atomic<bool> m_done { false };
};

struct IoUringScheduler {
public:
    IoUringContext* parent { nullptr };

private:
    friend auto tag_invoke(di::Tag<di::execution::schedule>, IoUringScheduler const& self) -> ScheduleSender;

    constexpr friend auto operator==(IoUringScheduler const&, IoUringScheduler const&) -> bool = default;
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

struct ConnectSender {
public:
    using is_sender = void;

    using CompletionSignatures = di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent { nullptr };
    int file_descriptor { -1 };
    net::UnixAddress address;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, int file_descriptor, net::UnixAddress address, Rec receiver)
                : m_parent(parent), m_file_descriptor(file_descriptor), m_receiver(di::move(receiver)) {
                if (address.path().size() < 108 - 1) {
                    sockaddr_un addr = {};
                    addr.sun_family = 1;
                    di::copy(address.path(), addr.sun_path);
                    m_address = addr;
                }
            }

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // If the path was too long, fail fast.
                    if (!m_address) {
                        di::execution::set_error(di::move(m_receiver), di::Error(dius::PosixError::FilenameTooLong));
                        return;
                    }

                    // Enqueue io_uring sqe with the close request.
                    enqueue_io_operation(m_parent, this, [&](auto* sqe) {
                        sqe->opcode = IORING_OP_CONNECT;
                        sqe->fd = m_file_descriptor;
                        sqe->addr = (u64) di::addressof(m_address.value());
                        sqe->off = sizeof(m_address.value());
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

            IoUringContext* m_parent { nullptr };
            int m_file_descriptor { -1 };
            di::Optional<sockaddr_un> m_address;
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, ConnectSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.file_descriptor, di::move(self.address),
                                          di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, ConnectSender const& self) {
        return Env(self.parent);
    }
};

struct BindSender {
public:
    using is_sender = void;

    using CompletionSignatures = di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent { nullptr };
    int file_descriptor { -1 };
    net::UnixAddress address;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, int file_descriptor, net::UnixAddress address, Rec receiver)
                : m_parent(parent), m_file_descriptor(file_descriptor), m_receiver(di::move(receiver)) {
                if (address.path().size() < 108 - 1) {
                    sockaddr_un addr = {};
                    addr.sun_family = 1;
                    di::copy(address.path(), addr.sun_path);
                    m_address = addr;
                }
            }

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // If the path was too long, fail fast.
                    if (!m_address) {
                        di::execution::set_error(di::move(m_receiver), di::Error(dius::PosixError::FilenameTooLong));
                        return;
                    }

                    // io_uring doesn't support bind() and listen()...
                    auto result = system::system_call<int>(system::Number::bind, m_file_descriptor,
                                                           di::addressof(m_address.value()), sizeof(m_address.value()));
                    if (!result.has_value()) {
                        di::execution::set_error(di::move(m_receiver), di::Error(di::move(result).error()));
                    } else {
                        di::execution::set_value(di::move(m_receiver));
                    }
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                enqueue_operation(self.m_parent, di::addressof(self));
            }

            IoUringContext* m_parent { nullptr };
            int m_file_descriptor { -1 };
            di::Optional<sockaddr_un> m_address;
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, BindSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.file_descriptor, di::move(self.address),
                                          di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, BindSender const& self) {
        return Env(self.parent);
    }
};

struct ListenSender {
public:
    using is_sender = void;

    using CompletionSignatures = di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent { nullptr };
    int file_descriptor { -1 };
    int count { 0 };

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, int file_descriptor, int count, Rec receiver)
                : m_parent(parent)
                , m_file_descriptor(file_descriptor)
                , m_count(count)
                , m_receiver(di::move(receiver)) {}

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // io_uring doesn't support bind() and listen()...
                    auto result = system::system_call<int>(system::Number::listen, m_file_descriptor, m_count);
                    if (!result.has_value()) {
                        di::execution::set_error(di::move(m_receiver), di::Error(di::move(result).error()));
                    } else {
                        di::execution::set_value(di::move(m_receiver));
                    }
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                enqueue_operation(self.m_parent, di::addressof(self));
            }

            IoUringContext* m_parent { nullptr };
            int m_file_descriptor { -1 };
            int m_count {};
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, ListenSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.file_descriptor, self.count, di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, ListenSender const& self) {
        return Env(self.parent);
    }
};

struct ShutdownSender {
public:
    using is_sender = void;

    using CompletionSignatures = di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent { nullptr };
    int file_descriptor { -1 };
    net::Shutdown how {};

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, int file_descriptor, net::Shutdown how, Rec receiver)
                : m_parent(parent), m_file_descriptor(file_descriptor), m_how(how), m_receiver(di::move(receiver)) {}

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // Enqueue io_uring sqe with the close request.
                    enqueue_io_operation(m_parent, this, [&](auto* sqe) {
                        sqe->opcode = IORING_OP_SHUTDOWN;
                        sqe->fd = m_file_descriptor;
                        sqe->len = (u32) m_how;
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

            IoUringContext* m_parent { nullptr };
            int m_file_descriptor { -1 };
            net::Shutdown m_how {};
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, ShutdownSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.file_descriptor, self.how, di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, ShutdownSender const& self) {
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

    auto parent() const -> IoUringContext* { return m_parent; }
    auto path() const -> di::Path const& { return m_path; }
    auto mode() const -> OpenMode { return m_mode; }
    auto create_mode() const -> u16 { return m_create_mode; }

    auto fd() const -> int { return m_fd; }
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

class AcceptSocket {
public:
    explicit AcceptSocket(int base_fd) : m_base_fd(base_fd) {}

    auto base_fd() const -> int { return m_base_fd; }

private:
    int m_base_fd { -1 };
};

template<typename Base>
class AsyncSocket
    : public di::Immovable
    , public Base {
public:
    template<typename... Args>
    requires(di::concepts::ConstructibleFrom<Base, Args...>)
    explicit AsyncSocket(IoUringContext* context, Args&&... args)
        : Base(di::forward<Args>(args)...), m_parent(context) {}

    auto parent() const -> IoUringContext* { return m_parent; }

    auto fd() const -> int { return m_fd; }
    void set_fd(int fd) { m_fd = fd; }

private:
    friend auto tag_invoke(di::Tag<di::execution::async_read_some>, AsyncSocket& self, di::Span<di::Byte> buffer,
                           di::Optional<u64>) {
        return ReadSomeSender { self.m_parent, self.m_fd, buffer, {} };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_write_some>, AsyncSocket& self, di::Span<di::Byte const> buffer,
                           di::Optional<u64>) {
        return WriteSomeSender { self.m_parent, self.m_fd, buffer, {} };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_bind>, AsyncSocket& self, net::UnixAddress address) {
        return BindSender { self.m_parent, self.m_fd, di::move(address) };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_connect>, AsyncSocket& self, net::UnixAddress address) {
        return ConnectSender { self.m_parent, self.m_fd, di::move(address) };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_listen>, AsyncSocket& self, int count) {
        return ListenSender { self.m_parent, self.m_fd, count };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_shutdown>, AsyncSocket& self, net::Shutdown how) {
        return ShutdownSender { self.m_parent, self.m_fd, how };
    }

    friend auto tag_invoke(di::Tag<di::execution::async_accept>, AsyncSocket& self) {
        return di::make_deferred<AsyncSocket<AcceptSocket>>(self.m_parent, self.m_fd);
    }

    IoUringContext* m_parent;
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

struct MakeSocketSender {
public:
    using is_sender = void;

    using AsyncSocket = linux::AsyncSocket<di::Void>;

    using CompletionSignatures = di::CompletionSignatures<di::SetValue(di::ReferenceWrapper<AsyncSocket>),
                                                          di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent;
    di::ReferenceWrapper<AsyncSocket> socket;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, di::ReferenceWrapper<AsyncSocket> socket, Rec&& receiver)
                : m_parent(parent), m_socket(socket), m_receiver(di::move(receiver)) {}

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // Enqueue io_uring sqe with the make socket request.
                    enqueue_io_operation(m_parent, this, [&](auto* sqe) {
                        sqe->opcode = IORING_OP_SOCKET;
                        sqe->fd = 1;
                        sqe->off = 1;
                    });
                }
            }

            void did_complete(io_uring::CQE const* cqe) override {
                if (cqe->res < 0) {
                    di::execution::set_error(di::move(m_receiver), di::Error(PosixError(-cqe->res)));
                } else {
                    m_socket.get().set_fd(cqe->res);
                    di::execution::set_value(di::move(m_receiver), di::ref(m_socket));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                enqueue_operation(self.m_parent, di::addressof(self));
            }

            IoUringContext* m_parent;
            di::ReferenceWrapper<AsyncSocket> m_socket;
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, MakeSocketSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.socket, di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, MakeSocketSender const& self) {
        return Env(self.parent);
    }
};

struct AcceptSender {
public:
    using is_sender = void;

    using AsyncSocket = linux::AsyncSocket<AcceptSocket>;

    using CompletionSignatures = di::CompletionSignatures<di::SetValue(di::ReferenceWrapper<AsyncSocket>),
                                                          di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent;
    di::ReferenceWrapper<AsyncSocket> socket;

private:
    template<typename Rec>
    struct OperationStateT {
        struct Type : OperationStateBase {
            explicit Type(IoUringContext* parent, di::ReferenceWrapper<AsyncSocket> socket, Rec&& receiver)
                : m_parent(parent), m_socket(socket), m_receiver(di::move(receiver)) {}

            void execute() override {
                if (di::execution::get_stop_token(m_receiver).stop_requested()) {
                    di::execution::set_stopped(di::move(m_receiver));
                } else {
                    // Enqueue io_uring sqe with the make socket request.
                    enqueue_io_operation(m_parent, this, [&](auto* sqe) {
                        sqe->opcode = IORING_OP_ACCEPT;
                        sqe->fd = m_socket.get().base_fd();
                    });
                }
            }

            void did_complete(io_uring::CQE const* cqe) override {
                if (cqe->res < 0) {
                    di::execution::set_error(di::move(m_receiver), di::Error(PosixError(-cqe->res)));
                } else {
                    m_socket.get().set_fd(cqe->res);
                    di::execution::set_value(di::move(m_receiver), di::ref(m_socket));
                }
            }

        private:
            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                enqueue_operation(self.m_parent, di::addressof(self));
            }

            IoUringContext* m_parent;
            di::ReferenceWrapper<AsyncSocket> m_socket;
            [[no_unique_address]] Rec m_receiver;
        };
    };

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<di::ReceiverOf<CompletionSignatures> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::connect>, AcceptSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.socket, di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, AcceptSender const& self) {
        return Env(self.parent);
    }
};

template<typename Object, typename CreateSend>
struct RunSender {
public:
    using is_sender = di::SequenceTag;

    using CompletionSignatures =
        di::CompletionSignatures<di::SetValue(di::ReferenceWrapper<Object>), di::SetError(di::Error), di::SetStopped()>;

    IoUringContext* parent;
    di::ReferenceWrapper<Object> object;

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

                auto base() const& -> Rec const& { return *m_receiver; }
                auto base() && -> Rec&& { return di::move(*m_receiver); }

            private:
                Rec* m_receiver;
            };

            explicit Type(IoUringContext* parent, di::ReferenceWrapper<Object> object, Rec&& receiver)
                : m_parent(parent), m_object(object), m_receiver(di::move(receiver)) {}

        private:
            using NextSender = di::meta::NextSenderOf<Rec, CreateSend>;
            using Op1 = di::meta::ConnectResult<NextSender, Rec1>;
            using Op2 = di::meta::ConnectResult<CloseSender, Rec2>;

            void finish_phase1() {
                auto& op = m_op.template emplace<2>(di::DeferConstruct([&] {
                    return di::execution::connect(CloseSender(m_parent, m_object.get().fd()),
                                                  Rec2(di::addressof(m_receiver)));
                }));
                di::execution::start(op);
            }

            friend void tag_invoke(di::Tag<di::execution::start>, Type& self) {
                auto& op = self.m_op.template emplace<1>(di::DeferConstruct([&] {
                    return di::execution::connect(
                        di::execution::set_next(self.m_receiver, CreateSend(self.m_parent, self.m_object)),
                        Rec1([&self] {
                            return self.finish_phase1();
                        }));
                }));
                di::execution::start(op);
            }

            IoUringContext* m_parent;
            di::ReferenceWrapper<Object> m_object;
            [[no_unique_address]] Rec m_receiver;
            DI_IMMOVABLE_NO_UNIQUE_ADDRESS di::Variant<di::Void, Op1, Op2> m_op;
        };
    };

    template<typename Receiver>
    using OperationState = di::meta::Type<OperationStateT<Receiver>>;

    template<
        di::ReceiverOf<di::CompletionSignatures<di::SetValue(), di::SetError(di::Error), di::SetStopped()>> Receiver>
    friend auto tag_invoke(di::Tag<di::execution::subscribe>, RunSender self, Receiver receiver) {
        return OperationState<Receiver> { self.parent, self.object, di::move(receiver) };
    }

    constexpr friend auto tag_invoke(di::Tag<di::execution::get_env>, RunSender const& self) {
        return di::execution::make_env(Env(self.parent),
                                       di::execution::with(di::execution::get_sequence_cardinality, di::c_<1ZU>));
    }
};

inline auto tag_invoke(di::Tag<di::execution::run>, AsyncFile& self) {
    return RunSender<AsyncFile, OpenSender> { self.parent(), self };
}

inline auto tag_invoke(di::Tag<di::execution::run>, AsyncSocket<di::Void>& self) {
    return RunSender<AsyncSocket<di::Void>, MakeSocketSender> { self.parent(), self };
}

inline auto tag_invoke(di::Tag<di::execution::run>, AsyncSocket<AcceptSocket>& self) {
    return RunSender<AsyncSocket<AcceptSocket>, AcceptSender> { self.parent(), self };
}

inline auto tag_invoke(di::Tag<di::execution::schedule>, IoUringScheduler const& self) -> ScheduleSender {
    return { self.parent };
}

inline auto tag_invoke(di::Tag<di::execution::async_open>, IoUringScheduler const& self, di::Path path, OpenMode mode,
                       u16 create_mode) {
    return di::make_deferred<AsyncFile>(self.parent, di::move(path), mode, create_mode);
}

inline auto tag_invoke(di::Tag<di::execution::async_open>, IoUringScheduler const& self, di::Path path, OpenMode mode) {
    return di::make_deferred<AsyncFile>(self.parent, di::move(path), mode, 0666);
}

inline auto tag_invoke(di::Tag<di::execution::async_make_socket>, IoUringScheduler const& self) {
    return di::make_deferred<AsyncSocket<di::Void>>(self.parent);
}

inline auto IoUringContext::create() -> di::Result<IoUringContext> {
    return IoUringContext(TRY(io_uring::IoUringHandle::create()));
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

inline auto get_scheduler(IoUringContext* context) -> IoUringScheduler {
    return context->get_scheduler();
}

inline auto IoUringContext::get_scheduler() -> IoUringScheduler {
    return IoUringScheduler(this);
}
}
