#ifdef __linux__
#include <di/execution/algorithm/when_all.h>
#include <di/execution/io/async_net.h>
#include <di/execution/io/ipc_protocol.h>
#include <di/reflect/prelude.h>
#include <dius/io_context.h>
#include <dius/ipc.h>
#include <dius/test/prelude.h>

namespace ipc {
struct ClientMessage1 {
    di::String s;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ClientMessage1>) {
        return di::make_fields(di::field<"s", &ClientMessage1::s>);
    }
};

struct ClientMessage2 {
    struct Reply {
        int x;
        int y;

        constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<Reply>) {
            return di::make_fields(di::field<"x", &Reply::x>, di::field<"y", &Reply::y>);
        }
    };

    int x;
    int y;
    int z;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ClientMessage2>) {
        return di::make_fields(di::field<"x", &ClientMessage2::x>, di::field<"y", &ClientMessage2::y>,
                               di::field<"z", &ClientMessage2::z>);
    }
};

struct ServerMessage {
    int z;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ServerMessage>) {
        return di::make_fields(di::field<"z", &ServerMessage::z>);
    }
};

using MyProtocol = di::Protocol<di::meta::List<ClientMessage1, ClientMessage2>, di::meta::List<ServerMessage>>;

static void basic() {
    auto context = *di::create<dius::IoContext>();

    // auto context = *di::create<dius::IoContext>();
    // auto scheduler = context.get_scheduler();
    //
    // asdfjasl this sdfasdf
    // namespace ex = di::execution;
    //
    // auto message = "Hello, World!"_sv;
    //
    // auto executed = false;
    // auto server = dius::ipc_server<MyProtocol>("dius_test_ipc"_ts, di::ipc::Receive(di::overload(
    //                                                        [&](ClientMessage2 m2) {
    //                                                            return ClientMessage2::Reply { m2.x + m2.y + m2.z, 2
    //                                                            };
    //                                                        },
    //                                                        [&](ClientMessage1 m1) {
    //                                                            ASSERT_EQ(m1.s, message);
    //                                                            executed = true;
    //                                                        })));
    //
    // auto client = dius::ipc_client<MyProtocol>("dius_test_ipc"_ts, di::ipc::Transmit([&](auto connection) ->
    // di::Lazy<> {
    //                                    co_await di::send(connection, ClientMessage1 { message.to_owned() });
    //
    //                                    auto r = co_await di::send(connection, ClientMessage2 { 1, 2, 3 });
    //                                    ASSERT_EQ(r.x, 6);
    //                                    ASSERT_EQ(r.y, 2);
    //
    // co_await ex::async_shutdown(socket, dius::net::Shutdown::ReadWrite);
    //                                    co_return {};
    //                                }));
    //
    // auto result = di::sync_wait_on(context, ex::when_all(di::move(server), di::move(client)));
    // ASSERT(result);
    // ASSERT(executed);
}

TEST(ipc, basic)
}
#endif
