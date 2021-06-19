#pragma once

namespace Ext {
enum class StreamResult {
    Success,
    Error,
    NeedsMoreInput,
    NeedsMoreOutputSpace,
};

enum class StreamFlushMode {
    NoFlush,
    BlockFlush,
    StreamFlush,
};
}
