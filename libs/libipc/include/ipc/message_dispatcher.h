#pragma once

#include <ipc/endpoint.h>

namespace IPC {

class Stream;

class MessageDispatcher {
public:
    virtual ~MessageDispatcher();
    virtual void handle_error() = 0;
    virtual void handle_incoming_data(Endpoint&, Stream&) = 0;

    virtual void handle_send_error() { handle_error(); }

    template<ConcreteMessage T>
    void send(Endpoint& endpoint, const T& message) {
        if (!endpoint.send<T>(message)) {
            handle_send_error();
        }
    }
};

}
