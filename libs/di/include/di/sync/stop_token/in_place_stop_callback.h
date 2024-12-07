#pragma once

#include "di/sync/stop_token/in_place_stop_token.h"

namespace di::sync {
template<typename Callback>
class InPlaceStopCallback : public detail::InPlaceStopCallbackBase {
public:
    using CallbackType = Callback;

    template<typename C>
    requires(concepts::ConstructibleFrom<Callback, C>)
    explicit InPlaceStopCallback(InPlaceStopToken token, C&& callback)
        : detail::InPlaceStopCallbackBase(token.m_source,
                                          [](void* self) {
                                              function::invoke(
                                                  util::move(static_cast<InPlaceStopCallback*>(self)->m_callback));
                                          })
        , m_callback(util::forward<C>(callback)) {
        if (m_parent) {
            if (!m_parent->try_add_callback(this)) {
                function::invoke(util::move(m_callback));
            }
        }
    }

    InPlaceStopCallback(InPlaceStopCallback&&) = delete;

    ~InPlaceStopCallback() {
        if (m_parent && !m_already_executed.load(MemoryOrder::Acquire)) {
            m_parent->remove_callback(this);
        }
    }

private:
    Callback m_callback;
};

template<typename Callback>
InPlaceStopCallback(InPlaceStopToken, Callback) -> InPlaceStopCallback<Callback>;
}
