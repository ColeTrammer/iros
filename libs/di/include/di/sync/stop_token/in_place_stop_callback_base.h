#pragma once

#include "di/container/intrusive/prelude.h"
#include "di/sync/atomic.h"
#include "di/sync/stop_token/forward_declaration.h"

namespace di::sync::detail {
class InPlaceStopCallbackBase : public container::IntrusiveListNode<> {
private:
    friend class ::di::sync::InPlaceStopSource;

    using ErasedCallback = void (*)(void*);

protected:
    explicit InPlaceStopCallbackBase(InPlaceStopSource const* parent, ErasedCallback execute)
        : m_parent(parent), m_execute(execute) {}

    InPlaceStopSource const* m_parent { nullptr };
    ErasedCallback m_execute { nullptr };
    Atomic<bool> m_already_executed { false };
    Atomic<bool*> m_did_destruct_in_same_thread { nullptr };

private:
    void execute() { m_execute(this); }
};
}
