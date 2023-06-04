#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/fs/debug_file.h>

namespace iris {
di::AnySenderOf<usize> tag_invoke(di::Tag<read_file>, DebugFile&, UserspaceBuffer<byte> buffer) {
    if (buffer.empty()) {
        return 0;
    }

    auto byte = 0_b;
    TRY(global_state().input_wait_queue.wait([&] {
        auto has_data = global_state().input_data_queue.pop();
        if (has_data) {
            byte = *has_data;
            return true;
        }
        return false;
    }));

    return TRY(buffer.write({ &byte, 1 }));
}

di::AnySenderOf<usize> tag_invoke(di::Tag<write_file>, DebugFile& self, UserspaceBuffer<byte const> data) {
    auto guard = di::ScopedLock(self.m_lock);
    TRY(data.copy_in_chunks<64>([&](di::Span<byte> chunk) -> Expected<void> {
        for (auto byte : chunk) {
            log_output_byte(byte);
        }
        return {};
    }));
    return data.size();
}
}
