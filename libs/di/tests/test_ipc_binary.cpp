#include <di/container/vector/vector.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/just_from.h>
#include <di/execution/algorithm/sync_wait.h>
#include <di/execution/algorithm/use_resources.h>
#include <di/execution/io/async_read_some.h>
#include <di/execution/io/async_write_some.h>
#include <di/execution/io/ipc_binary.h>
#include <di/execution/io/ipc_protocol.h>
#include <di/execution/sequence/ignore_all.h>
#include <di/execution/sequence/then_each.h>
#include <di/function/overload.h>
#include <di/function/tag_invoke.h>
#include <di/io/read_exactly.h>
#include <di/io/vector_reader.h>
#include <di/io/vector_writer.h>
#include <di/io/write_exactly.h>
#include <di/serialization/binary_serializer.h>
#include <dius/test/prelude.h>

namespace ipc_binary {
struct AsyncReader {
    di::VectorReader<> sync_reader;

    friend auto tag_invoke(di::Tag<di::execution::async_read_some>, AsyncReader& self, di::Span<byte> buffer, auto) {
        return di::execution::just_from([&self, buffer] -> di::Result<usize> {
            return di::read_some(self.sync_reader, buffer);
        });
    }
};

struct AsyncWriter {
    di::VectorWriter<> sync_writer;

    friend auto tag_invoke(di::Tag<di::execution::async_write_some>, AsyncWriter& self, di::Span<byte const> buffer,
                           auto) {
        return di::execution::just_from([&self, buffer] -> di::Result<usize> {
            return di::write_some(self.sync_writer, buffer);
        });
    }
};

struct ClientMessage1 {
    int x;
    int y;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ClientMessage1>) {
        return di::make_fields(di::field<"x", &ClientMessage1::x>, di::field<"y", &ClientMessage1::y>);
    }
};

struct ClientMessage2 {
    struct Response {
        int x;
        int y;

        constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<Response>) {
            return di::make_fields(di::field<"x", &Response::x>, di::field<"y", &Response::y>);
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

static void send() {
    auto read = AsyncReader {};
    auto write = AsyncWriter {};

    auto client = di::execution::use_resources(
        [](auto connection) {
            return di::send(connection, ClientMessage1 { 1, 2 });
        },
        di::execution::ipc_binary_connect_to_client<MyProtocol>(di::ref(read), di::ref(write), di::identity));
    ASSERT(di::sync_wait(client));

    ASSERT_EQ(write.sync_writer.vector().size(), 20);
}

static void recv() {
    auto client_read = AsyncReader {};
    auto client_write = AsyncWriter {};

    auto client = di::execution::use_resources(
        [](auto connection) {
            return di::send(connection, ClientMessage1 { 1, 2 }) | di::execution::let_value([connection] {
                       return di::send(connection, ClientMessage2 { 3, 4, 5 });
                   });
        },
        di::execution::ipc_binary_connect_to_client<MyProtocol>(di::ref(client_read), di::ref(client_write),
                                                                di::identity));
    ASSERT(di::sync_wait(client));

    ASSERT_EQ(client_write.sync_writer.vector().size(), 44);

    auto read = AsyncReader { di::VectorReader<> { di::move(client_write).sync_writer.vector() } };
    auto write = AsyncWriter {};

    auto m1_count = 0;
    auto m2_count = 0;
    auto server = di::execution::use_resources(
        [](auto) {
            return di::execution::just();
        },
        di::execution::ipc_binary_connect_to_client<MyProtocol>(di::ref(read), di::ref(write),
                                                                di::execution::then_each(di::overload(
                                                                    [&](ClientMessage1) {
                                                                        ++m1_count;
                                                                    },
                                                                    [&](ClientMessage2) {
                                                                        ++m2_count;
                                                                    }))));
    ASSERT(di::sync_wait(server));

    ASSERT_EQ(m1_count, 1);
    ASSERT_EQ(m2_count, 1);
}

TEST(ipc_binary, send)
TEST(ipc_binary, recv)
}
