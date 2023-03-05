#pragma once

namespace iris {
enum class ShutdownStatus {
    Error,
    Intended,
};

[[noreturn]] void hard_shutdown(ShutdownStatus status);
}
