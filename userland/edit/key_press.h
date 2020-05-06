#pragma once

struct KeyPress {
    enum Modifier {
        Shift = 1,
        Alt = 2,
        Control = 4,
    };

    enum Key {
        // Ascii keys are mapped to themselves
        LeftArrow = 1000,
        RightArrow,
        UpArrow,
        DownArrow,
        Home,
        End,

        Backspace = 2000,
        Delete,
        Enter,
        Insert,
        Escape,
        PageUp,
        PageDown,

        F0 = 3000,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
    };

    int modifiers;
    int key;
};