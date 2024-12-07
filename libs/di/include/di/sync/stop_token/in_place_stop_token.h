#pragma once

#include "di/sync/stop_token/in_place_stop_source.h"

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

    [[nodiscard]] auto stop_requested() const -> bool { return !!m_source && m_source->stop_requested(); }
    [[nodiscard]] auto stop_possible() const -> bool { return !!m_source; }

    [[nodiscard]] auto operator==(InPlaceStopToken const&) const -> bool = default;

private:
    InPlaceStopSource const* m_source { nullptr };
};

inline auto InPlaceStopSource::get_stop_token() const -> InPlaceStopToken {
    return InPlaceStopToken { this };
}
}
