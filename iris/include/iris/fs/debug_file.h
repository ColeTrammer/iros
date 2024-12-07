#pragma once

#include "di/execution/any/any_sender.h"
#include "iris/core/interruptible_spinlock.h"
#include "iris/core/print.h"
#include "iris/fs/file.h"

namespace iris {
struct DebugFile {
private:
    friend auto tag_invoke(di::Tag<read_file>, DebugFile&, UserspaceBuffer<byte> buffer) -> di::AnySenderOf<usize>;

    friend auto tag_invoke(di::Tag<write_file>, DebugFile& self, UserspaceBuffer<byte const> data)
        -> di::AnySenderOf<usize>;

    InterruptibleSpinlock m_lock;
};
}
