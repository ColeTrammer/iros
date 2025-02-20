#pragma once

namespace dius {
using ProcessId = int;

enum class Signal {
    WindowChange = 1,
    Alarm = 2,
    Interrupt = 3,
    Pipe = 4,
    Hangup = 5,
};
}
