#pragma once

#include <ipc/stream.h>

namespace IPC {

class MessageDispatcher {
public:
    virtual ~MessageDispatcher();
    virtual void handle_error() = 0;
    virtual void handle_incoming_data(Stream&) = 0;
};

}
