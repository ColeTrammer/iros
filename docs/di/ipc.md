# Inter-Process Communication

## Purpose

The library module aims to provide a set of tools for inter-process communication. This builds on top of the
asynchronous execution facilities provided by the [execution module](execution.md).

## Conceptual Overview

The library aims to be agnostic over the underlying communication mechanism. The core concept is that of a connection,
which is a bidirectional communication channel between two processes. The connection is represented by a pair of
objects, one which is async writable and the other which is async readable. The library provides a set of functions
which serialize and deserialize messages sent over the connection.

### Sending Messages

Sending a message is done by serializing the message into a buffer, and then writing the buffer to the connection. This
is represented as a async sender, which will complete when the message has been sent.

### Receiving Messages

As the library is fully asynchronous, received messages are represented by a async-sequence, which produces a sequence
of messages. Using this mechanism, a server is constructed by "mapping" this sequence to a sender, which in normal
circumstances will send a message back to the client.

### Sending and Receiving Messages

In most cases, a client will want to send a message and then wait for a reply. This is represented by chaining a sender
which waits for a reply to a sender which sends the message. This makes the reply directly available, and so the client
can also use it in the continuation of the async operation.

### Connection Management

The library provides a set of customization points for protocols to implement connection management. Starting a
connection as a client is a matter of getting a sender which resolves to a connection. Starting a connection as a server
involves mapping a sequence of incoming connection requests to a real connection object.

The next question is how to terminate a connection. The will be done by assoicating a stop token with the connection.
When a stop is requested, the receiving sequence will stop producing messages, and complete after closing the resources
associated with the connection.

Another concern is broadcasting messages to multiple clients. This requires maintaining a list of connections, and then
sending a message to each one concurrently.

## Usage

### Defining a Message Type

To use the automatic serialization and deserialization facilities, the message type must be defined with static
[reflection](static_reflection.md) information included.

```cpp
#include <di/reflect/prelude.h>

// Define a simple message type.
struct ClientMessage1 {
    int x;
    int y;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ClientMessage1>) {
        return di::make_fields(
            di::field<"x", &ClientMessage1::x>,
            di::field<"y", &ClientMessage1::y>
        );
    }
};

// Define a message type which requires a reply.
struct ClientMessage2 {
    struct Reply {
        int x;
        int y;

        constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<Reply>) {
            return di::make_fields(
                di::field<"x", &Reply::x>,
                di::field<"y", &Reply::y>
            );
        }
    };

    int x;
    int y;
    int z;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ClientMessage2>) {
        return di::make_fields(
            di::field<"x", &ClientMessage2::x>,
            di::field<"y", &ClientMessage2::y>,
            di::field<"z", &ClientMessage2::z>
        );
    }
};

// Define a message type which is only sent from the server.
struct ServerMessage {
    int z;

    constexpr friend auto tag_invoke(di::Tag<di::reflect>, di::InPlaceType<ServerMessage>) {
        return di::make_fields(
            di::field<"z", &ServerMessage::z>
        );
    }
};

// Define a message "protocol", which consists of a list of messages which can be sent from the client, and a list of
// messages which can be sent from the server.
using MyProtocol = di::Protocol<di::meta::List<ClientMessage1, ClientMessage2>, di::meta::List<ServerMessage>>;
```

From this example, we can see that the message types are defined as simple structs. The static reflection information
allows the library to automatically serialize and deserialize the messages.

There can also be a nested `Reply` type, which is used to represent the reply to a message. This is useful for messages
which require a reply. The library uses this information to automatically chain waiting for the reply when sending a
message (this doesn't block since everything is asynchronous).

Additionally, a protocol is defined by 2 lists of messages. This implies that every connection is either a client or a
server. In a symmetric protocol, both lists will be the same.

### Creating a Connection

In order to create a connection, the library makes use of a variadic argument factory, which simulates named arguments.
Since connections are bidirectional, the factory will always accept a `transmit()` object, which is a function which is
passed the connection token and returns a sender, and a `receive()` object, which is a transformation which accepts a
received message and the connection token, and returns a sender. For ease of use, the second argument is optional, and
for messages which expect replys, the returned sender must send the reply as a value. The factory may also accept
additional arguments, which are defined on a per-protocol basis.

```cpp
using MyProtocol = /* ... */;

auto reader = /* ... */;
auto writer = /* ... */;

// Example 1: only need to send messages.
auto ex1 = di::ipc_binary_connect_to_server<MyProtocol>(
    di::Transmit([](auto token) {
        return di::execution::send(token, /* ... */);
    })
);

// Example 2: only need to receive messages.
auto ex2 = di::ipc_binary_connect_to_client<MyProtocol>(
    di::Receive([](auto&& message, auto token) {
        return di::execution::send(token, /* ... */);
    })
);

// Example 3: need to send and receive messages, but don't need the connection to send replys.
auto ex3 = di::ipc_binary_connect_to_server<MyProtocol>(
    di::Transmit([](auto token) {
        return di::execution::send(token, /* ... */);
    }),
    di::Receive([]<typename T>(T message) {
        if constexpr (di::concepts::SameAs<T, ClientMessage1>) {
            /* Send a reply */
            return di::execution::just(ClientMessage1::Reply { /* ... */});
        } else {
            /* Do something with the message. */
            return di::execution::just();
        }
    })
);
```

The reason this API is designed this way is to ensure the lifetime of the connection is managed correctly. The
transmitter and receiver are both passed the connection token, which is managed by the outer sender. Additionally, the
received messsage system is managed by the outer sender, which allows for waiting for replies to messages.

## Synchronization

Because the model involves has a separate read and write stream, there is no need for synchronization between the two.
The read component is a sequence, and since it reading from a single stream, it will not produce messages out of order.
However, the write component is a sender, which means that it can be invoked concurrently. This means that the messages
sent over the connection may be interleaved. Because there is only a single underlying stream, the library has to
synchronize the writes to ensure that data chunks from separate messages are not interleaved. Additionally, if a message
expects a reply, the library must synchronize with the read stream so it knows where to route decoded messages to. Note
that reply messages are not sent to the read stream, but are instead routed directly to the sender which is waiting for
the reply.
