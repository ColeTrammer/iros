#pragma once

#include <iris/core/interruptible_spinlock.h>
#include <iris/fs/file.h>

namespace iris {
struct DebugFile {
private:
    friend Expected<usize> tag_invoke(di::Tag<read_file>, DebugFile& self, di::Span<di::Byte> data);

    friend Expected<usize> tag_invoke(di::Tag<write_file>, DebugFile& self, di::Span<di::Byte const> data) {
        auto guard = di::ScopedLock(self.m_lock);
        for (auto byte : data) {
            log_output_byte(byte);
        }
        return data.size();
    }

    InterruptibleSpinlock m_lock;
};
}
