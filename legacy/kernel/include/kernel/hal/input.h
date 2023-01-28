#ifndef _KERNEL_HAL_INPUT_H
#define _KERNEL_HAL_INPUT_H 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum key {
    KEY_NULL,
    KEY_ESC,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_MINUS,
    KEY_EQUALS,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_Q,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_LEFT_SQUARE_BRACKET,
    KEY_RIGHT_SQUARE_BRACKET,
    KEY_ENTER,
    KEY_LEFT_CONTROL,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_SEMICOLON,
    KEY_SINGLE_QUOTE,
    KEY_BACK_TICK,
    KEY_LEFT_SHIFT,
    KEY_BACK_SLASH,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_COMMA,
    KEY_DOT,
    KEY_FORWARD_SLASH,
    KEY_RIGHT_SHIFT,
    KEY_NUMPAD_STAR,
    KEY_LEFT_ALT,
    KEY_SPACE,
    KEY_CAPSLOCK,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_NUMLOCK,
    KEY_SCROLL_LOCK,
    KEY_NUMPAD_7,
    KEY_NUMPAD_8,
    KEY_NUMPAD_9,
    KEY_NUMPAD_MINUS,
    KEY_NUMPAD_4,
    KEY_NUMPAD_5,
    KEY_NUMPAD_6,
    KEY_NUMPAD_PLUS,
    KEY_NUMPAD_1,
    KEY_NUMPAD_2,
    KEY_NUMPAD_3,
    KEY_NUMPAD_0,
    KEY_NUMPAD_DOT,
    KEY_F11,
    KEY_F12,
    KEY_PREV_TRACK,
    KEY_NEXT_TRACX,
    KEY_NUMPAD_ENTER,
    KEY_RIGHT_CONTROL,
    KEY_MUTE,
    KEY_CALC,
    KEY_PLAY,
    KEY_STOP,
    KEY_VOLUMNE_DOWN,
    KEY_VOLUME_UP,
    KEY_WWW,
    KEY_NUMPAD_FORWARD_SLASH,
    KEY_RIGHT_ALT,
    KEY_HOME,
    KEY_CURSOR_UP,
    KEY_PAGE_UP,
    KEY_CURSOR_LEFT,
    KEY_CURSOR_RIGHT,
    KEY_END,
    KEY_CURSOR_DOWN,
    KEY_PAGE_DOWN,
    KEY_INSERT,
    KEY_DELETE,
    KEY_LEFT_GUI,
    KEY_RIGHT_GUI,
    KEY_APPS,
    KEY_ACPI_POWER,
    KEY_ACPI_SLEEP,
    KEY_ACPI_WAKE,
    KEY_WWW_SEARCH,
    KEY_WWW_FAVORITES,
    KEY_WWW_REFRESH,
    KEY_WWW_STOP,
    KEY_WWW_FORWARD,
    KEY_WWW_BACK,
    KEY_MY_COMPUTER,
    KEY_EMAIL,
    KEY_MEDIA_SELECT
};

struct key_event {
    char ascii;

    enum key key;

#define KEY_UP         (1U << 1U)
#define KEY_DOWN       (1U << 2U)
#define KEY_CONTROL_ON (1U << 3U)
#define KEY_SHIFT_ON   (1U << 4U)
#define KEY_ALT_ON     (1U << 5U)
    unsigned int flags;
};

enum mouse_button {
    MOUSE_BUTTON_LEFT = 1,
    MOUSE_BUTTON_RIGHT = 2,
    MOUSE_BUTTON_MIDDLE = 4,
};

enum scale_mode {
    SCALE_DELTA,
    SCALE_ABSOLUTE,
};

struct mouse_event {
    enum scale_mode scale_mode;
    int dx;
    int dy;
    int dz;
    int buttons;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _KERNEL_HAL_INPUT_H */
