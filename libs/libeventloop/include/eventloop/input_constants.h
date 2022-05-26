#pragma once

#include <liim/enum.h>

namespace App {
namespace MouseButton {
    enum {
        None = 0,
        Left = 1,
        Right = 2,
        Middle = 4,
    };
}

namespace KeyModifier {
    enum {
        Shift = 1,
        Alt = 2,
        Control = 4,
        Meta = 8,
    };
}
}

// clang-format off
#define __APP_ENUM_KEY(m) \
    m(None) \
    m(A) \
    m(B) \
    m(C) \
    m(D) \
    m(E) \
    m(F) \
    m(G) \
    m(H) \
    m(I) \
    m(J) \
    m(K) \
    m(L) \
    m(M) \
    m(N) \
    m(O) \
    m(P) \
    m(Q) \
    m(R) \
    m(S) \
    m(T) \
    m(U) \
    m(V) \
    m(W) \
    m(X) \
    m(Y) \
    m(Z) \
    m(_1) \
    m(_2) \
    m(_3) \
    m(_4) \
    m(_5) \
    m(_6) \
    m(_7) \
    m(_8) \
    m(_9) \
    m(_0) \
    m(Enter) \
    m(Escape) \
    m(Backspace) \
    m(Tab) \
    m(Space) \
    m(Minus) \
    m(Equals) \
    m(LeftBracket) \
    m(RightBracket) \
    m(Backslash) \
    m(NonUS_Pound) \
    m(SemiColon) \
    m(Quote) \
    m(Backtick) \
    m(Comma) \
    m(Period) \
    m(Slash) \
    m(CapsLock) \
    m(F1) \
    m(F2) \
    m(F3) \
    m(F4) \
    m(F5) \
    m(F6) \
    m(F7) \
    m(F8) \
    m(F9) \
    m(F10) \
    m(F11) \
    m(F12) \
    m(PrintScreen) \
    m(ScrollLock) \
    m(Pause) \
    m(Insert) \
    m(Home) \
    m(PageUp) \
    m(Delete) \
    m(End) \
    m(PageDown) \
    m(RightArrow) \
    m(LeftArrow) \
    m(DownArrow) \
    m(UpArrow) \
    m(NumLock) \
    m(Numpad_Slash) \
    m(Numpad_Star) \
    m(Numpad_Minus) \
    m(Numpad_Plus) \
    m(Numpad_Enter) \
    m(Numpad_1) \
    m(Numpad_2) \
    m(Numpad_3) \
    m(Numpad_4) \
    m(Numpad_5) \
    m(Numpad_6) \
    m(Numpad_7) \
    m(Numpad_8) \
    m(Numpad_9) \
    m(Numpad_0) \
    m(Numpad_Period) \
    m(NonUS_Backslash) \
    m(Application) \
    m(Power) \
    m(Numpad_Equals) \
    m(F13) \
    m(F14) \
    m(F15) \
    m(F16) \
    m(F17) \
    m(F18) \
    m(F19) \
    m(F20) \
    m(F21) \
    m(F22) \
    m(F23) \
    m(F24) \
    m(LeftControl) \
    m(LeftShift) \
    m(LeftAlt) \
    m(LeftMeta) \
    m(RightControl) \
    m(RightShift) \
    m(RightAlt) \
    m(RightMeta)
// clang-format on

LIIM_ENUM(App, Key, __APP_ENUM_KEY)
