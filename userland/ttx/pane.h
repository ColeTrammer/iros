#pragma once

#include "di/container/string/string_view.h"
#include "di/container/vector/vector.h"
#include "di/function/container/function.h"
#include "di/sync/atomic.h"
#include "di/sync/synchronized.h"
#include "di/vocab/error/result.h"
#include "di/vocab/pointer/box.h"
#include "dius/sync_file.h"
#include "dius/thread.h"
#include "dius/tty.h"
#include "renderer.h"
#include "ttx/key_event.h"
#include "ttx/mouse.h"
#include "ttx/paste_event.h"
#include "ttx/terminal.h"

namespace ttx {
class Pane {
public:
    static auto create(di::Vector<di::TransparentStringView> command, dius::tty::WindowSize size)
        -> di::Result<di::Box<Pane>>;

    explicit Pane(dius::SyncFile pty_controller)
        : m_pty_controller(di::move(pty_controller)), m_terminal(m_pty_controller) {}
    ~Pane();

    auto draw(Renderer& renderer) -> RenderedCursor;

    auto event(KeyEvent const& event) -> bool;
    auto event(MouseEvent const& event) -> bool;
    auto event(FocusEvent const& event) -> bool;
    auto event(PasteEvent const& event) -> bool;

    void resize(dius::tty::WindowSize const& size);

    // Application controlled callback when the internal process exits.
    di::Function<void()> did_exit;

private:
    di::Atomic<bool> m_done { false };
    di::Optional<MousePosition> m_last_mouse_position;
    dius::SyncFile m_pty_controller;
    di::Synchronized<Terminal> m_terminal;
    dius::Thread m_process_thread;
    dius::Thread m_reader_thread;
};
}
