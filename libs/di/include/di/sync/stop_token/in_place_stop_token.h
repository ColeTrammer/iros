#pragma once

#include <di/sync/stop_token/in_place_stop_source.h>

namespace di::sync {
class InPlaceStopToken {
private:
    friend class InPlaceStopSource;

    template<typename>
    friend class InPlaceStopCallback;

    explicit InPlaceStopToken(InPlaceStopSource const* source) : m_source(source) {}

public:
    template<typename Callback>
    using CallbackType = InPlaceStopCallback<Callback>;

    InPlaceStopToken() = default;
    ~InPlaceStopToken() = default;

    [[nodiscard]] bool stop_requested() const { return !!m_source && m_source->stop_requested(); }
    [[nodiscard]] bool stop_possible() const { return !!m_source; }

    [[nodiscard]] bool operator==(InPlaceStopToken const&) const = default;

private:
    InPlaceStopSource const* m_source { nullptr };
};

inline InPlaceStopToken InPlaceStopSource::get_stop_token() const {
    return InPlaceStopToken { this };
}
}