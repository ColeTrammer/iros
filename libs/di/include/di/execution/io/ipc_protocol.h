#pragma once

#include <di/container/allocator/allocator.h>
#include <di/execution/concepts/sender.h>
#include <di/execution/io/async_read_some.h>
#include <di/execution/io/async_write_some.h>
#include <di/function/tag_invoke.h>
#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/meta/function.h>
#include <di/meta/language.h>
#include <di/meta/list.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/in_place_template.h>
#include <di/util/move.h>
#include <di/util/named_arguments.h>

namespace di::concepts {
template<typename T>
concept MessageWithReply = requires { typename T::Reply; } && !di::SameAs<T, typename T::Reply>;
}

namespace di::meta {
namespace detail {
    template<typename T>
    struct MessageReplyHelper {
        using Type = void;
    };

    template<concepts::MessageWithReply T>
    struct MessageReplyHelper<T> {
        using Type = typename T::Reply;
    };
}

template<typename T>
using MessageReply = Type<detail::MessageReplyHelper<T>>;
}

namespace di::execution {
template<concepts::TypeList ClientMessages, concepts::TypeList ServerMessages>
struct Protocol {
    using Client = ClientMessages;
    using Server = ServerMessages;
};
}

namespace di::meta {
template<typename T>
requires(requires { typename T::Protocol; })
using Protocol = typename T::Protocol;
}

namespace di::concepts {
template<typename T>
concept HasProtocol = InstanceOf<meta::Protocol<T>, execution::Protocol>;
}

template<typename T>
constexpr inline bool is_client = requires { typename T::is_client; };

template<typename T>
constexpr inline bool is_server = requires { typename T::is_server; };

namespace di::meta {
namespace detail {
    struct MustBeClientOrServer {};

    template<concepts::TypeList Messages>
    using MessagesReplies = Filter<Transform<Messages, meta::Quote<MessageReply>>, Not<SameAs<void>>>;
}

template<concepts::HasProtocol T>
using MessageTypesWithoutReplies =
    Conditional<is_client<T>, typename meta::Protocol<T>::Client,
                Conditional<is_server<T>, typename meta::Protocol<T>::Server, detail::MustBeClientOrServer>>;

template<concepts::HasProtocol T>
using PeerMessageTypesWithoutReplies =
    Conditional<is_client<T>, typename meta::Protocol<T>::Server,
                Conditional<is_server<T>, typename meta::Protocol<T>::Client, detail::MustBeClientOrServer>>;

template<concepts::HasProtocol T>
using MessageTypes = Concat<MessageTypesWithoutReplies<T>, detail::MessagesReplies<PeerMessageTypesWithoutReplies<T>>>;

template<concepts::HasProtocol T>
using AllMessageTypesWithoutReplies = Concat<typename meta::Protocol<T>::Client, typename meta::Protocol<T>::Server>;
}

namespace di::concepts {
template<typename T, typename Message>
concept Reply = meta::Contains<meta::detail::MessagesReplies<meta::AllMessageTypesWithoutReplies<T>>, Message>;
}

namespace di::execution {
namespace send_ns {
    struct Function {
        template<concepts::HasProtocol SendToken, typename Message, typename... Args,
                 typename MessageTypes = meta::MessageTypes<SendToken>>
        requires(meta::Contains<MessageTypes, Message> && concepts::TagInvocable<Function, SendToken, Message, Args...>)
        auto operator()(SendToken send_token, Message&& message, Args&&... args) const -> concepts::Sender auto {
            return tag_invoke(*this, di::move(send_token), di::forward<Message>(message), di::forward<Args>(args)...);
        }
    };
}

constexpr inline auto send = send_ns::Function {};

namespace ipc {
    template<concepts::AsyncReadable Reader>
    struct Receiver : NamedArgument<InPlaceTemplate<Receiver>, di::meta::RemoveReference<Reader>> {};

    template<typename Reader>
    Receiver(Reader&&) -> Receiver<Reader>;

    template<concepts::AsyncWritable Writer>
    struct Transmitter : NamedArgument<InPlaceTemplate<Transmitter>, Writer> {};

    template<typename Writer>
    Transmitter(Writer&&) -> Transmitter<di::meta::RemoveReference<Writer>>;

    template<concepts::AsyncReadable RW>
    requires(concepts::AsyncWritable<RW>)
    struct ReceiverTransmitter : NamedArgument<InPlaceTemplate<ReceiverTransmitter>, RW> {};

    template<typename RW>
    ReceiverTransmitter(RW&&) -> ReceiverTransmitter<di::meta::RemoveReference<RW>>;

    template<concepts::MoveConstructible F>
    struct Transmit : NamedArgument<InPlaceTemplate<Transmit>, F> {};

    template<typename F>
    Transmit(F&&) -> Transmit<F>;

    template<concepts::MoveConstructible F>
    struct Receive : NamedArgument<InPlaceTemplate<Receive>, F> {};

    template<typename F>
    Receive(F&&) -> Receive<F>;

    template<concepts::Allocator Alloc>
    requires(concepts::CopyConstructible<Alloc>)
    struct Allocator : NamedArgument<InPlaceTemplate<Allocator>, Alloc> {};

    template<typename Alloc>
    Allocator(Alloc&) -> Allocator<Alloc>;

    using BaseNamedArguments =
        meta::List<InPlaceTemplate<Receiver>, InPlaceTemplate<Transmitter>, InPlaceTemplate<ReceiverTransmitter>,
                   InPlaceTemplate<Transmit>, InPlaceTemplate<Receive>, InPlaceTemplate<Allocator>>;
}
}

namespace di {
namespace ipc = execution::ipc;

using execution::Protocol;
using execution::send;
}
