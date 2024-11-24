#pragma once

#include <di/execution/io/ipc_protocol.h>
#include <di/meta/core.h>
namespace dius {
namespace ipc {
    template<typename Proto>
    struct IpcServer {};
}

template<di::concepts::InstanceOf<di::Protocol> Proto>
constexpr inline auto ipc_server = ipc::IpcServer<Proto> {};
}
