#pragma once

#include <eventloop/unix_socket.h>
#include <ipc/message.h>
#include <liim/function.h>
#include <liim/maybe.h>

namespace IPC {

class MessageDispatcher;

class Endpoint : public App::Object {
    APP_OBJECT(Endpoint)

public:
    virtual ~Endpoint() override;

    void set_dispatcher(SharedPtr<MessageDispatcher> dispatcher) { m_dispatcher = move(dispatcher); }
    void set_socket(SharedPtr<App::UnixSocket> socket);

    template<ConcreteMessage T>
    bool send(const T& val) {
        char buffer[4096];
        if (val.serialization_size() > sizeof(buffer)) {
            return false;
        }
        Stream stream(buffer, sizeof(buffer));
        if (!val.serialize(stream)) {
            return false;
        }
        return send_impl(*reinterpret_cast<const Message*>(buffer));
    }

    template<ConcreteMessage T>
    Maybe<T> wait_for_response() {
        auto message = wait_for_response_impl(static_cast<uint32_t>(T::message_type()));
        if (!message) {
            return {};
        }
        Stream stream(reinterpret_cast<char*>(message.get()), message->size);
        T val;
        if (!val.deserialize(stream)) {
            return {};
        }
        return { move(val) };
    }

    template<ConcreteMessage S, ConcreteMessage R>
    Maybe<R> send_then_wait(const S& message) {
        if (!send(message)) {
            return {};
        }
        return wait_for_response<R>();
    }

    Function<void(Endpoint&)> on_disconnect;

private:
    void handle_messages();
    bool read_from_socket();
    bool send_impl(const Message& message);
    UniquePtr<Message> wait_for_response_impl(uint32_t type);

    SharedPtr<App::UnixSocket> m_socket;
    SharedPtr<MessageDispatcher> m_dispatcher;
    Vector<UniquePtr<Message>> m_messages;
    String m_path;
};

}
