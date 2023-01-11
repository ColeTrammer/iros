#pragma once

#include <di/container/intrusive/prelude.h>
#include <di/sync/atomic.h>
#include <di/sync/synchronized.h>

namespace di::sync {
class InPlaceStopToken;

class InPlaceStopSource {
public:
    InPlaceStopSource() {}

    InPlaceStopSource(InPlaceStopSource&&) = delete;

    ~InPlaceStopSource() {}

    [[nodiscard]] InPlaceStopToken get_stop_token() const;
    [[nodiscard]] bool stop_possible() const { return !stop_requested(); }
    [[nodiscard]] bool stop_requested() const { return m_stop_requested.load(MemoryOrder::Acquire); }

    bool request_stop() { return !m_stop_requested.exchange(true, MemoryOrder::Acquire); }

private:
    Atomic<bool> m_stop_requested;
};
}