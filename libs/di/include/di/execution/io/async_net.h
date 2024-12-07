#pragma once

#include "di/execution/concepts/scheduler.h"
#include "di/execution/concepts/sender_of.h"
#include "di/execution/interface/run.h"
#include "di/execution/receiver/set_value.h"
#include "di/function/invoke.h"
#include "di/function/tag_invoke.h"

namespace di::execution {
namespace async_net_ns {
    struct AsyncMakeSocket {
        template<concepts::Scheduler Sched, typename... ExtraArgs>
        requires(concepts::TagInvocable<AsyncMakeSocket, Sched, ExtraArgs...>)
        constexpr auto operator()(Sched&& sched, ExtraArgs&&... extra_args) const {
            static_assert(concepts::AsyncResource<
                              meta::InvokeResult<meta::TagInvokeResult<AsyncMakeSocket, Sched, ExtraArgs...>>>,
                          "async_make_socket() customizations must return a deferred di::AsyncResource instance.");
            return tag_invoke(*this, di::forward<Sched>(sched), di::forward<ExtraArgs>(extra_args)...);
        }
    };

    struct AsyncAccept {
        template<typename Socket, typename... ExtraArgs>
        requires(concepts::TagInvocable<AsyncAccept, Socket, ExtraArgs...>)
        constexpr auto operator()(Socket&& socket, ExtraArgs&&... extra_args) const {
            static_assert(
                concepts::AsyncResource<meta::InvokeResult<meta::TagInvokeResult<AsyncAccept, Socket, ExtraArgs...>>>,
                "async_accept() customizations must return a deferred di::AsyncResource instance.");
            return tag_invoke(*this, di::forward<Socket>(socket), di::forward<ExtraArgs>(extra_args)...);
        }
    };

    struct AsyncBind {
        template<typename Socket, typename... ExtraArgs>
        requires(concepts::TagInvocable<AsyncBind, Socket, ExtraArgs...>)
        constexpr auto operator()(Socket&& socket, ExtraArgs&&... extra_args) const
            -> concepts::SenderOf<SetValue()> auto {
            static_assert(concepts::SenderOf<meta::TagInvokeResult<AsyncBind, Socket, ExtraArgs...>, SetValue()>,
                          "async_bind() customizations must return a sender of void.");
            return tag_invoke(*this, di::forward<Socket>(socket), di::forward<ExtraArgs>(extra_args)...);
        }
    };

    struct AsyncConnect {
        template<typename Socket, typename... ExtraArgs>
        requires(concepts::TagInvocable<AsyncConnect, Socket, ExtraArgs...>)
        constexpr auto operator()(Socket&& socket, ExtraArgs&&... extra_args) const
            -> concepts::SenderOf<SetValue()> auto {
            static_assert(concepts::SenderOf<meta::TagInvokeResult<AsyncConnect, Socket, ExtraArgs...>, SetValue()>,
                          "async_connect() customizations must return a sender of void.");
            return tag_invoke(*this, di::forward<Socket>(socket), di::forward<ExtraArgs>(extra_args)...);
        }
    };

    struct AsyncListen {
        template<typename Socket, typename... ExtraArgs>
        requires(concepts::TagInvocable<AsyncListen, Socket, ExtraArgs...>)
        constexpr auto operator()(Socket&& socket, ExtraArgs&&... extra_args) const
            -> concepts::SenderOf<SetValue()> auto {
            static_assert(concepts::SenderOf<meta::TagInvokeResult<AsyncListen, Socket, ExtraArgs...>, SetValue()>,
                          "async_listen() customizations must return a sender of void.");
            return tag_invoke(*this, di::forward<Socket>(socket), di::forward<ExtraArgs>(extra_args)...);
        }
    };

    struct AsyncShutdown {
        template<typename Socket, typename... ExtraArgs>
        requires(concepts::TagInvocable<AsyncShutdown, Socket, ExtraArgs...>)
        constexpr auto operator()(Socket&& socket, ExtraArgs&&... extra_args) const
            -> concepts::SenderOf<SetValue()> auto {
            static_assert(concepts::SenderOf<meta::TagInvokeResult<AsyncShutdown, Socket, ExtraArgs...>, SetValue()>,
                          "async_shutdown() customizations must return a sender of void.");
            return tag_invoke(*this, di::forward<Socket>(socket), di::forward<ExtraArgs>(extra_args)...);
        }
    };
}

constexpr inline auto async_make_socket = async_net_ns::AsyncMakeSocket {};
constexpr inline auto async_accept = async_net_ns::AsyncAccept {};
constexpr inline auto async_bind = async_net_ns::AsyncBind {};
constexpr inline auto async_connect = async_net_ns::AsyncConnect {};
constexpr inline auto async_listen = async_net_ns::AsyncListen {};
constexpr inline auto async_shutdown = async_net_ns::AsyncShutdown {};
}
