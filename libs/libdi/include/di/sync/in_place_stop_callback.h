#pragma once

#include <di/sync/in_place_stop_token.h>

namespace di::sync {
template<typename Callback>
class InPlaceStopCallback {
public:
    using CallbackType = Callback;

    template<typename C>
    requires(concepts::ConstructibleFrom<Callback, C>)
    explicit InPlaceStopCallback(InPlaceStopToken, C&& callback) : m_callback(util::forward<C>(callback)) {}

    InPlaceStopCallback(InPlaceStopCallback&&) = delete;

    ~InPlaceStopCallback() = default;

private:
    Callback m_callback;
};

template<typename Callback>
InPlaceStopCallback(InPlaceStopToken, Callback) -> InPlaceStopCallback<Callback>;
}