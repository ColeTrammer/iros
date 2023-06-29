#pragma once

#include <di/execution/concepts/sender.h>
#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/meta/list.h>
#include <di/util/move.h>

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
}

template<concepts::HasProtocol T>
using MessageTypes =
    Conditional<is_client<T>, typename meta::Protocol<T>::Client,
                Conditional<is_server<T>, typename meta::Protocol<T>::Server, detail::MustBeClientOrServer>>;
}

namespace di::execution {
namespace send_ns {
    struct Function {
        template<concepts::HasProtocol SendToken, typename Message,
                 typename MessageTypes = meta::MessageTypes<SendToken>>
        requires(meta::Contains<MessageTypes, Message> && concepts::TagInvocable<Function, SendToken, Message>)
        concepts::Sender auto operator()(SendToken send_token, Message&& message) const {
            return tag_invoke(*this, di::move(send_token), di::forward<Message>(message));
        }
    };
}

constexpr inline auto send = send_ns::Function {};
}

namespace di {
using execution::Protocol;
using execution::send;
}
