#pragma once

#include <di/bit/endian/little_endian.h>
#include <di/container/allocator/allocator.h>
#include <di/container/vector/vector.h>
#include <di/execution/algorithm/just.h>
#include <di/execution/algorithm/just_from.h>
#include <di/execution/algorithm/let.h>
#include <di/execution/interface/run.h>
#include <di/execution/io/async_read_some.h>
#include <di/execution/io/async_write_exactly.h>
#include <di/execution/io/async_write_some.h>
#include <di/execution/io/ipc_protocol.h>
#include <di/function/make_deferred.h>
#include <di/function/monad/monad_try.h>
#include <di/function/tag_invoke.h>
#include <di/io/vector_writer.h>
#include <di/io/write_exactly.h>
#include <di/math/intcmp/representable_as.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/list.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/platform/custom.h>
#include <di/serialization/binary_serializer.h>
#include <di/serialization/serialize.h>
#include <di/types/byte.h>
#include <di/util/addressof.h>
#include <di/util/bit_cast.h>
#include <di/util/reference_wrapper.h>
#include <di/vocab/array/array.h>
#include <di/vocab/error/result.h>

namespace di::execution {
namespace ipc_binary_ns {
    struct Client {
        using is_server = void;
    };

    struct Server {
        using is_client = void;
    };

    struct MessageHeader {
        LittleEndian<u32> message_type;
        LittleEndian<u32> message_number;
        LittleEndian<u32> message_size;
    };

    template<typename Proto, typename Read, typename Write, typename Alloc>
    struct ConnectionDataT {
        struct Type {
            [[no_unique_address]] Read read;
            [[no_unique_address]] Write write;
            [[no_unique_address]] Alloc allocator;
            Vector<byte, Alloc> send_buffer;
            u32 message_number { 0 };
        };
    };

    template<typename Proto, typename Read, typename Write, typename Alloc>
    using ConnectionData = meta::Type<ConnectionDataT<Proto, Read, Write, Alloc>>;

    template<typename Proto, typename Read, typename Write, typename Alloc, typename ClientOrServer>
    struct ConnectionTokenT {
        struct Type : ClientOrServer {
            using Serializer = BinarySerializer<VectorWriter<Vector<byte, Alloc>>>;
            using Protocol = Proto;

            explicit Type(ConnectionData<Proto, Read, Write, Alloc>* data_) : data(data_) {}

            template<concepts::Serializable<Serializer> T, typename U = meta::Decay<T>,
                     typename MessageList = meta::MessageTypes<Type>>
            requires(concepts::DecayConstructible<T> && meta::Contains<MessageList, U>)
            friend auto tag_invoke(Tag<send>, Type self, T&& message) {
                constexpr auto message_index = meta::Lookup<U, MessageList>;
                static_assert(math::representable_as<u32>(message_index),
                              "There can be at most 2^32 messages in a protocol.");

                // FIXME: acquire a async mutex first.
                return just_from([message = util::forward<T>(message), data = self.data] -> Result<> {
                           // Serialize the message. We re-use the send buffer to prevent extra allocations.
                           auto send_buffer = di::move(data->send_buffer);
                           send_buffer.clear();

                           auto total_size = sizeof(MessageHeader) + serialize_size(binary_format, message);
                           if constexpr (concepts::FallibleAllocator<Alloc>) {
                               DI_TRY(send_buffer.reserve(total_size));
                           } else {
                               send_buffer.reserve(total_size);
                           }

                           auto writer = VectorWriter(di::move(send_buffer));
                           auto message_header = MessageHeader {
                               .message_type = static_cast<u32>(message_index),
                               .message_number = data->message_number++,
                               .message_size = total_size,
                           };
                           auto as_bytes = di::bit_cast<Array<byte, sizeof(MessageHeader)>>(message_header);
                           DI_TRY(write_exactly(writer, as_bytes.span()));
                           DI_TRY(serialize_binary(writer, message));
                           data->send_buffer = di::move(writer).vector();
                           return {};
                       }) |
                       let_value([data = self.data] {
                           return async_write_exactly(data->write, data->send_buffer.span());
                       });
            }

            ConnectionData<Proto, Read, Write, Alloc>* data;
        };
    };

    template<typename Proto, typename Read, typename Write, typename Alloc, typename ClientOrServer>
    using ConnectionToken = meta::Type<ConnectionTokenT<Proto, Read, Write, Alloc, ClientOrServer>>;

    template<typename Proto, typename Read, typename Write, typename Alloc, typename ClientOrServer>
    struct ConnectionT {
        struct Type {
            explicit Type(Read read, Write write, Alloc allocator)
                : data(di::move(read), di::move(write), di::move(allocator)) {}

            [[no_unique_address]] ConnectionData<Proto, Read, Write, Alloc> data;

            friend auto tag_invoke(Tag<run>, Type& self) {
                return just(ConnectionToken<Proto, Read, Write, Alloc, ClientOrServer>(di::addressof(self.data)));
            }
        };
    };

    template<typename Proto, typename Read, typename Write, typename Alloc>
    using ClientConnection =
        meta::Type<ConnectionT<Proto, meta::RemoveCVRef<Read>, meta::RemoveCVRef<Write>, meta::Decay<Alloc>, Client>>;

    template<typename Proto, typename Read, typename Write, typename Alloc>
    using ServerConnection =
        meta::Type<ConnectionT<Proto, meta::RemoveCVRef<Read>, meta::RemoveCVRef<Write>, meta::Decay<Alloc>, Server>>;

    template<concepts::InstanceOf<Protocol> Proto>
    struct ConnectToClientFunction {
        template<typename Comm, concepts::Allocator Alloc = DefaultAllocator>
        requires(concepts::AsyncReadable<Comm> && concepts::AsyncWritable<Comm> && concepts::CopyConstructible<Comm>)
        auto operator()(Comm&& channel, Alloc&& allocator = {}) const {
            return (*this)(auto(channel), auto(channel), di::forward<Alloc>(allocator));
        }

        template<concepts::AsyncReadable ReadComm, concepts::AsyncWritable WriteComm,
                 concepts::Allocator Alloc = DefaultAllocator>
        auto operator()(ReadComm&& read, WriteComm&& write, Alloc&& allocator = {}) const {
            return make_deferred<ServerConnection<Proto, ReadComm, WriteComm, Alloc>>(
                di::forward<ReadComm>(read), di::forward<WriteComm>(write), di::forward<Alloc>(allocator));
        }
    };

    template<concepts::InstanceOf<Protocol> Proto>
    struct ConnectToServerFunction {
        template<typename Comm, concepts::Allocator Alloc = DefaultAllocator>
        requires(concepts::AsyncReadable<Comm> && concepts::AsyncWritable<Comm> && concepts::CopyConstructible<Comm>)
        auto operator()(Comm&& channel, Alloc&& allocator = {}) const {
            return (*this)(auto(channel), auto(channel), di::forward<Alloc>(allocator));
        }

        template<concepts::AsyncReadable ReadComm, concepts::AsyncWritable WriteComm,
                 concepts::Allocator Alloc = DefaultAllocator>
        auto operator()(ReadComm&& read, WriteComm&& write, Alloc&& allocator = {}) const {
            return make_deferred<ClientConnection<Proto, ReadComm, WriteComm, Alloc>>(
                di::forward<ReadComm>(read), di::forward<WriteComm>(write), di::forward<Alloc>(allocator));
        }
    };
}

template<concepts::InstanceOf<Protocol> Proto>
constexpr inline auto ipc_binary_connect_to_client = ipc_binary_ns::ConnectToClientFunction<Proto> {};

template<concepts::InstanceOf<Protocol> Proto>
constexpr inline auto ipc_binary_connect_to_server = ipc_binary_ns::ConnectToServerFunction<Proto> {};
}

namespace di {
using execution::ipc_binary_connect_to_client;
using execution::ipc_binary_connect_to_server;
}
