#include "di/container/vector/vector.h"
#include "di/execution/algorithm/sync_wait.h"
#include "di/execution/algorithm/use_resources.h"
#include "di/execution/algorithm/when_all.h"
#include "di/execution/coroutine/prelude.h"
#include "di/execution/interface/schedule.h"
#include "di/execution/io/async_net.h"
#include "di/execution/io/async_read_exactly.h"
#include "di/execution/io/async_write_exactly.h"
#include "di/execution/io/async_write_some.h"
#include "di/execution/io/ipc_binary.h"
#include "di/execution/io/ipc_protocol.h"
#include "di/reflect/field.h"
#include "di/reflect/prelude.h"
#include "di/vocab/span/as_bytes.h"
#include "di/vocab/span/as_writable_bytes.h"
#include "dius/io_context.h"
#include "dius/net/address.h"
#include "dius/net/socket.h"
#include "dius/print.h"
#include "dius/test/prelude.h"

#ifdef __linux__
namespace socket_test {

static void unix_scoket() {
    auto context = *di::create<dius::IoContext>();
    auto scheduler = context.get_scheduler();

    namespace ex = di::execution;

    auto address = dius::net::UnixAddress("\0dius_test_socket"_ts);

    auto message = "Hello, World!"_ts;

    auto executed = false;
    auto server = ex::use_resources(
        [&](auto& passive_socket) -> di::Lazy<> {
            co_await ex::async_bind(passive_socket, address.clone());
            co_await ex::async_listen(passive_socket, 1);

            co_await ex::use_resources(
                [&](auto& socket) -> di::Lazy<> {
                    auto buffer = di::Vector<char> {};
                    buffer.resize(message.size());

                    co_await ex::async_read_exactly(socket, di::as_writable_bytes(buffer.span()));

                    auto received = di::move(buffer) | di::to<di::TransparentString>();
                    ASSERT_EQ(received, message);

                    executed = true;
                    co_return {};
                },
                ex::async_accept(passive_socket));

            co_return {};
        },
        ex::async_make_socket(scheduler));

    auto client = ex::use_resources(
        [&](auto& socket) -> di::Lazy<> {
            co_await ex::async_connect(socket, address.clone());

            co_await ex::async_write_exactly(socket, di::as_bytes(message.span()));

            co_return {};
        },
        ex::async_make_socket(scheduler));

    auto result = di::sync_wait_on(context, ex::when_all(di::move(server), di::move(client)));
    ASSERT(result);
    ASSERT(executed);
}

struct ClientMessage1 {
    di::String s;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ClientMessage1>) {
        return di::make_fields<"ClientMessage1">(di::field<"s", &ClientMessage1::s>);
    }
};

struct ClientMessage2 {
    struct Reply {
        int x;
        int y;

        constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<Reply>) {
            return di::make_fields<"ClientMessage2::Reply">(di::field<"x", &Reply::x>, di::field<"y", &Reply::y>);
        }
    };

    int x;
    int y;
    int z;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ClientMessage2>) {
        return di::make_fields<"ClientMessage2">(di::field<"x", &ClientMessage2::x>, di::field<"y", &ClientMessage2::y>,
                                                 di::field<"z", &ClientMessage2::z>);
    }
};

struct ServerMessage {
    int z;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ServerMessage>) {
        return di::make_fields<"ServerMessage">(di::field<"z", &ServerMessage::z>);
    }
};

using MyProtocol = di::Protocol<di::meta::List<ClientMessage1, ClientMessage2>, di::meta::List<ServerMessage>>;

static void ipc_binary() {
    auto context = *di::create<dius::IoContext>();
    auto scheduler = context.get_scheduler();

    namespace ex = di::execution;

    auto address = dius::net::UnixAddress("\0dius_test_ipc_binary"_ts);

    auto message = "Hello, World!"_sv;

    auto executed = false;
    auto server = ex::use_resources(
        [&](auto& passive_socket) -> di::Lazy<> {
            co_await ex::async_bind(passive_socket, address.clone());
            co_await ex::async_listen(passive_socket, 1);

            co_await ex::use_resources(
                [&](auto& socket) -> di::Lazy<> {
                    co_await ex::ipc_binary_connect_to_client<MyProtocol>(
                        di::ipc::ReceiverTransmitter(socket), di::ipc::Receive(di::overload(
                                                                  [&](ClientMessage2 m2) {
                                                                      return ClientMessage2::Reply { m2.x + m2.y + m2.z,
                                                                                                     2 };
                                                                  },
                                                                  [&](ClientMessage1 m1) {
                                                                      ASSERT_EQ(m1.s, message);
                                                                      executed = true;
                                                                  })));
                    co_return {};
                },
                ex::async_accept(passive_socket));

            co_return {};
        },
        ex::async_make_socket(scheduler));

    auto client = ex::use_resources(
        [&](auto& socket) -> di::Lazy<> {
            co_await ex::async_connect(socket, address.clone());

            co_await ex::ipc_binary_connect_to_server<MyProtocol>(
                di::ipc::ReceiverTransmitter(socket), di::ipc::Transmit([&](auto connection) -> di::Lazy<> {
                    co_await di::send(connection, ClientMessage1 { message.to_owned() });

                    auto r = co_await di::send(connection, ClientMessage2 { 1, 2, 3 });
                    ASSERT_EQ(r.x, 6);
                    ASSERT_EQ(r.y, 2);

                    co_await ex::async_shutdown(socket, dius::net::Shutdown::ReadWrite);
                    co_return {};
                }));

            co_return {};
        },
        ex::async_make_socket(scheduler));

    auto result = di::sync_wait_on(context, ex::when_all(di::move(server), di::move(client)));
    ASSERT(result);
    ASSERT(executed);
}

TEST(socket, unix_scoket)
TEST(socket, ipc_binary)
}
#endif
