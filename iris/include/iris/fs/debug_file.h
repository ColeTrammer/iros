#pragma once

#include <iris/core/interruptible_spinlock.h>
#include <iris/core/print.h>
#include <iris/fs/file.h>

namespace iris {
struct DebugFile {
private:
    friend Expected<usize> tag_invoke(di::Tag<read_file>, DebugFile&, WritableUserspaceBuffer data);

    friend Expected<usize> tag_invoke(di::Tag<write_file>, DebugFile& self, ReadonlyUserspaceBuffer data);

    InterruptibleSpinlock m_lock;
};
}
