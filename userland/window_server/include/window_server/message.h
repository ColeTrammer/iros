#pragma once

#include <ipc/gen.h>
#include <liim/pointers.h>
#include <liim/string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/hal/input.h>

typedef uint64_t wid_t;

namespace WindowServer {

enum WindowType {
    Application,
    Frameless,
};

}

namespace LIIM {

template<>
struct Traits<scroll_state> {
    static constexpr bool is_simple() { return true; }
};

template<>
struct Traits<mouse_button_state> {
    static constexpr bool is_simple() { return true; }
};

template<>
struct Traits<key_event> {
    static constexpr bool is_simple() { return true; }
};

template<>
struct Traits<WindowServer::WindowType> {
    static constexpr bool is_simple() { return true; }
};

}

namespace WindowServer {

// clang-format off
IPC_MESSAGES(Client, 
    (CreateWindowRequest,
        (int, x),
        (int, y),
        (int, width),
        (int, height),
        (String, name),
        (WindowType, type),
        (wid_t, parent_id),
        (bool, has_alpha),
    ),
    (RemoveWindowRequest,
        (wid_t, wid),
    ),
    (ChangeWindowVisibilityRequest,
        (wid_t, wid),
        (int, x),
        (int, y),
        (bool, visible),
    ),
    (SwapBufferRequest,
        (wid_t, wid),
    ),
    (WindowReadyToResizeMessage,
        (wid_t, wid),
    ),
    (WindowRenameRequest,
        (wid_t, wid),
        (String, name),
    ),
    (ChangeThemeRequest,
        (String, path),
    ),
)

IPC_MESSAGES(Server,
    (CreateWindowResponse,
        (wid_t, wid),
        (size_t, size),
        (String, path),
    ),
    (RemoveWindowResponse,
        (bool, success),
    ),
    (ChangeWindowVisibilityResponse,
        (wid_t, wid),
        (bool, success),
    ),
    (KeyEventMessage,
        (wid_t, wid),
        (key_event, event),
    ),
    (MouseEventMessage,
        (wid_t, wid),
        (int, x),
        (int, y),
        (scroll_state, scroll),
        (mouse_button_state, left),
        (mouse_button_state, right),
    ),
    (WindowDidResizeMessage,
        (wid_t, wid),
    ),
    (WindowReadyToResizeResponse,
        (wid_t, wid),
        (int, new_width),
        (int, new_height),
    ),
    (WindowClosedEventMessage,
        (wid_t, wid),
    ),
    (WindowStateChangeMessage,
        (wid_t, wid),
        (bool, active),
    ),
    (ChangeThemeResponse,
        (bool, success),
    ),
    (ThemeChangeMessage),
)
// clang-format on
}
