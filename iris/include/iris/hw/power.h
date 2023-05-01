#pragma once

namespace iris {
enum class ShutdownStatus {
    Error,
    Intended,
    DoubleFault,
};

[[noreturn]] void hard_shutdown(ShutdownStatus status);
}
