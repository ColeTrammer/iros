#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/fs/debug_file.h>

namespace iris {
Expected<usize> tag_invoke(di::Tag<read_file>, DebugFile&, di::Span<di::Byte> buffer) {
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

    buffer[0] = byte;
    return 1;
}
}
