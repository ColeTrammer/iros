#pragma once

#include <linux/types.h>

namespace dius {
using ProcessId = __kernel_pid_t;

enum class Signal {
    Hangup = 1,
    Interrupt,
    Quit,
    IllegalInstruction,
    Trap,
    Abort,
    IOTimeout = Abort,
    Bus,
    FloatingPointException,
    Kill,
    User1,
    SegmentationViolation,
    User2,
    Pipe,
    Alarm,
    Terminal,
    StackFault,
    Child,
    Continue,
    Stop,
    TerminalStop,
    TerminalInput,
    TerminalOutput,
    Urgent,
    XCPU,
    GraphicsSize,
    VTAlarm,
    Profile,
    WindowChange,
    IO,
    Poll = IO,
    Power,
    System,
};
}
