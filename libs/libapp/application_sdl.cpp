#ifdef __linux__
#include <app/application_sdl.h>
#include <app/window_sdl.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_hints.h>

// #define SDL_DEBUG

namespace App {
static void* thread_run_sdl(void*) {
    SDLApplication::the().run_sdl();
    return nullptr;
}

SDLApplication& SDLApplication::the() {
    auto& app = Application::the();
    assert(app.is_sdl_application());
    return static_cast<SDLApplication&>(app);
}

SDLApplication::SDLApplication() {
    initialize_palette(Palette::create_from_json(RESOURCE_ROOT "/usr/share/themes/default.json"));

    pthread_barrier_init(&m_sdl_init_barrier, nullptr, 2);
    pthread_create(&m_sdl_thread, nullptr, thread_run_sdl, nullptr);
    wait_for_sdl_init();
}

SDLApplication::~SDLApplication() {}

UniquePtr<PlatformWindow> SDLApplication::create_window(Window& window, int x, int y, int width, int height, String name, bool has_alpha,
                                                        WindowServer::WindowType type, wid_t parent_id) {
    return make_unique<SDLWindow>(window, x, y, width, height, move(name), has_alpha, type, parent_id);
}

void SDLApplication::on_sdl_window_event(const SDL_Event& event) {
    auto maybe_target_window = Window::find_by_wid(event.window.windowID);
    if (!maybe_target_window) {
        return;
    }

    auto& window = **maybe_target_window;
    switch (event.window.event) {
        case SDL_WINDOWEVENT_SHOWN:
        case SDL_WINDOWEVENT_HIDDEN:
            break;
        case SDL_WINDOWEVENT_CLOSE:
            EventLoop::queue_event(window.weak_from_this(), make_unique<WindowEvent>(WindowEvent::Type::Close));
            break;
        case SDL_WINDOWEVENT_EXPOSED:
            EventLoop::queue_event(window.weak_from_this(), make_unique<WindowEvent>(WindowEvent::Type::ForceRedraw));
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            EventLoop::queue_event(window.weak_from_this(), make_unique<WindowEvent>(WindowEvent::Type::DidResize));
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            EventLoop::queue_event(window.weak_from_this(), make_unique<WindowStateEvent>(true));
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            EventLoop::queue_event(window.weak_from_this(), make_unique<WindowStateEvent>(false));
            break;
        case SDL_WINDOWEVENT_MOVED:
            break;
        case SDL_WINDOWEVENT_TAKE_FOCUS:
            SDL_SetWindowInputFocus(static_cast<SDLWindow&>(window.platform_window()).sdl_window());
            break;
        default:
#ifdef SDL_DEBUG
            fprintf(stderr, "EVENT: %d\n", (int) event.window.event);
#endif /* SDL_DEBUG */
            break;
    }
}

static int translate_sdl_button(uint8_t button) {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return MouseButton::Left;
        case SDL_BUTTON_MIDDLE:
            return MouseButton::Middle;
        case SDL_BUTTON_RIGHT:
            return MouseButton::Right;
        default:
            return 0;
    }
}

static key translate_sdl_keycode(SDL_Keycode keycode) {
    switch (keycode) {
        case SDLK_RETURN:
            return KEY_ENTER;
        case SDLK_ESCAPE:
            return KEY_ESC;
        case SDLK_BACKSPACE:
            return KEY_BACKSPACE;
        case SDLK_TAB:
            return KEY_TAB;
        case SDLK_SPACE:
            return KEY_SPACE;
        case SDLK_QUOTE:
            return KEY_SINGLE_QUOTE;
        case SDLK_COMMA:
            return KEY_COMMA;
        case SDLK_MINUS:
            return KEY_MINUS;
        case SDLK_PERIOD:
            return KEY_DOT;
        case SDLK_SLASH:
            return KEY_FORWARD_SLASH;
        case SDLK_0:
            return KEY_0;
        case SDLK_1:
            return KEY_1;
        case SDLK_2:
            return KEY_2;
        case SDLK_3:
            return KEY_3;
        case SDLK_4:
            return KEY_4;
        case SDLK_5:
            return KEY_5;
        case SDLK_6:
            return KEY_6;
        case SDLK_7:
            return KEY_7;
        case SDLK_8:
            return KEY_8;
        case SDLK_9:
            return KEY_9;
        case SDLK_SEMICOLON:
            return KEY_SEMICOLON;
        case SDLK_EQUALS:
            return KEY_EQUALS;
        case SDLK_LEFTBRACKET:
            return KEY_LEFT_SQUARE_BRACKET;
        case SDLK_BACKSLASH:
            return KEY_BACK_SLASH;
        case SDLK_RIGHTBRACKET:
            return KEY_RIGHT_SQUARE_BRACKET;
        case SDLK_BACKQUOTE:
            return KEY_BACK_TICK;
        case SDLK_a:
            return KEY_A;
        case SDLK_b:
            return KEY_B;
        case SDLK_c:
            return KEY_C;
        case SDLK_d:
            return KEY_D;
        case SDLK_e:
            return KEY_E;
        case SDLK_f:
            return KEY_F;
        case SDLK_g:
            return KEY_G;
        case SDLK_h:
            return KEY_H;
        case SDLK_i:
            return KEY_I;
        case SDLK_j:
            return KEY_J;
        case SDLK_k:
            return KEY_K;
        case SDLK_l:
            return KEY_L;
        case SDLK_m:
            return KEY_M;
        case SDLK_n:
            return KEY_N;
        case SDLK_o:
            return KEY_O;
        case SDLK_p:
            return KEY_P;
        case SDLK_q:
            return KEY_Q;
        case SDLK_r:
            return KEY_R;
        case SDLK_s:
            return KEY_S;
        case SDLK_t:
            return KEY_T;
        case SDLK_u:
            return KEY_U;
        case SDLK_v:
            return KEY_V;
        case SDLK_w:
            return KEY_W;
        case SDLK_x:
            return KEY_X;
        case SDLK_y:
            return KEY_Y;
        case SDLK_z:
            return KEY_Z;
        case SDLK_CAPSLOCK:
            return KEY_CAPSLOCK;
        case SDLK_F1:
            return KEY_F1;
        case SDLK_F2:
            return KEY_F2;
        case SDLK_F3:
            return KEY_F3;
        case SDLK_F4:
            return KEY_F4;
        case SDLK_F5:
            return KEY_F5;
        case SDLK_F6:
            return KEY_F6;
        case SDLK_F7:
            return KEY_F7;
        case SDLK_F8:
            return KEY_F8;
        case SDLK_F9:
            return KEY_F9;
        case SDLK_F10:
            return KEY_F10;
        case SDLK_F11:
            return KEY_F11;
        case SDLK_F12:
            return KEY_F12;
        case SDLK_SCROLLLOCK:
            return KEY_SCROLL_LOCK;
        case SDLK_PAUSE:
            return KEY_WWW_STOP;
        case SDLK_INSERT:
            return KEY_INSERT;
        case SDLK_HOME:
            return KEY_HOME;
        case SDLK_PAGEUP:
            return KEY_PAGE_UP;
        case SDLK_DELETE:
            return KEY_DELETE;
        case SDLK_END:
            return KEY_END;
        case SDLK_PAGEDOWN:
            return KEY_PAGE_DOWN;
        case SDLK_RIGHT:
            return KEY_CURSOR_RIGHT;
        case SDLK_LEFT:
            return KEY_CURSOR_LEFT;
        case SDLK_DOWN:
            return KEY_CURSOR_DOWN;
        case SDLK_UP:
            return KEY_CURSOR_UP;
        case SDLK_KP_DIVIDE:
            return KEY_NUMPAD_FORWARD_SLASH;
        case SDLK_KP_MULTIPLY:
            return KEY_NUMPAD_STAR;
        case SDLK_KP_MINUS:
            return KEY_NUMPAD_MINUS;
        case SDLK_KP_PLUS:
            return KEY_NUMPAD_PLUS;
        case SDLK_KP_ENTER:
            return KEY_NUMPAD_ENTER;
        case SDLK_KP_1:
            return KEY_NUMPAD_1;
        case SDLK_KP_2:
            return KEY_NUMPAD_2;
        case SDLK_KP_3:
            return KEY_NUMPAD_3;
        case SDLK_KP_4:
            return KEY_NUMPAD_4;
        case SDLK_KP_5:
            return KEY_NUMPAD_5;
        case SDLK_KP_6:
            return KEY_NUMPAD_6;
        case SDLK_KP_7:
            return KEY_NUMPAD_7;
        case SDLK_KP_8:
            return KEY_NUMPAD_8;
        case SDLK_KP_9:
            return KEY_NUMPAD_9;
        case SDLK_KP_0:
            return KEY_NUMPAD_0;
        case SDLK_KP_PERIOD:
            return KEY_NUMPAD_DOT;
        case SDLK_APPLICATION:
            return KEY_APPS;
        case SDLK_POWER:
            return KEY_ACPI_POWER;
        case SDLK_SELECT:
            return KEY_MEDIA_SELECT;
        case SDLK_STOP:
            return KEY_WWW_STOP;
        case SDLK_AGAIN:
            return KEY_WWW_REFRESH;
        case SDLK_MUTE:
            return KEY_MUTE;
        case SDLK_VOLUMEUP:
            return KEY_VOLUME_UP;
        case SDLK_VOLUMEDOWN:
            return KEY_VOLUMNE_DOWN;
        case SDLK_LCTRL:
            return KEY_LEFT_CONTROL;
        case SDLK_LSHIFT:
            return KEY_LEFT_SHIFT;
        case SDLK_LALT:
            return KEY_LEFT_ALT;
        case SDLK_LGUI:
            return KEY_LEFT_GUI;
        case SDLK_RCTRL:
            return KEY_RIGHT_CONTROL;
        case SDLK_RSHIFT:
            return KEY_RIGHT_SHIFT;
        case SDLK_RALT:
            return KEY_RIGHT_ALT;
        case SDLK_RGUI:
            return KEY_RIGHT_GUI;
        case SDLK_AUDIONEXT:
            return KEY_NEXT_TRACX;
        case SDLK_AUDIOPREV:
            return KEY_PREV_TRACK;
        case SDLK_AUDIOSTOP:
            return KEY_STOP;
        case SDLK_AUDIOPLAY:
            return KEY_PLAY;
        case SDLK_AUDIOMUTE:
            return KEY_MUTE;
        case SDLK_MEDIASELECT:
            return KEY_MEDIA_SELECT;
        case SDLK_WWW:
            return KEY_WWW;
        case SDLK_MAIL:
            return KEY_EMAIL;
        case SDLK_CALCULATOR:
            return KEY_CALC;
        case SDLK_COMPUTER:
            return KEY_MY_COMPUTER;
        case SDLK_AC_SEARCH:
            return KEY_WWW_SEARCH;
        case SDLK_AC_BACK:
            return KEY_WWW_BACK;
        case SDLK_AC_FORWARD:
            return KEY_WWW_FORWARD;
        case SDLK_AC_STOP:
            return KEY_WWW_STOP;
        case SDLK_AC_REFRESH:
            return KEY_WWW_REFRESH;
        default:
            return (key) 0;
    }
}

static int translate_sdl_modifiers(uint16_t sdl_modifers) {
    int modifiers = 0;
    if (sdl_modifers & KMOD_SHIFT) {
        modifiers |= KEY_SHIFT_ON;
    }
    if (sdl_modifers & KMOD_ALT) {
        modifiers |= KEY_ALT_ON;
    }
    if (sdl_modifers & KMOD_CTRL) {
        modifiers |= KEY_CONTROL_ON;
    }
    return modifiers;
}

void SDLApplication::run_sdl() {
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    SDL_StartTextInput();

    pthread_barrier_wait(&m_sdl_init_barrier);

    for (;;) {
        SDL_Event event;
        SDL_WaitEvent(&event);

        switch (event.type) {
            case SDL_WINDOWEVENT:
                on_sdl_window_event(event);
                break;
            case SDL_KEYDOWN: {
                auto maybe_window = Window::find_by_wid(event.key.windowID);
                if (!maybe_window) {
                    break;
                }

                auto modifiers = translate_sdl_modifiers(event.key.keysym.mod) | KEY_DOWN;
                EventLoop::queue_event(*maybe_window, make_unique<KeyEvent>(0, translate_sdl_keycode(event.key.keysym.sym), modifiers));
                break;
            }
            case SDL_TEXTINPUT: {
                auto maybe_window = Window::find_by_wid(event.text.windowID);
                if (!maybe_window) {
                    break;
                }

                auto modifiers = translate_sdl_modifiers(SDL_GetModState()) | KEY_DOWN;
                EventLoop::queue_event(*maybe_window, make_unique<KeyEvent>(event.text.text[0], (key) 0, modifiers));
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                auto button = translate_sdl_button(event.button.button);
                if (!button) {
                    break;
                }

                auto buttons_down = Application::the().mouse_tracker().prev_buttons();
                if (event.button.state == SDL_PRESSED) {
                    buttons_down |= button;
                } else {
                    buttons_down &= ~button;
                }

                auto events = Application::the().mouse_tracker().notify_mouse_event(buttons_down, event.button.x, event.button.y, 0);
                auto maybe_target_window = Window::find_by_wid(event.button.windowID);
                if (!maybe_target_window) {
                    break;
                }

                for (auto& event : events) {
                    EventLoop::queue_event(*maybe_target_window, move(event));
                }
                break;
            }
            case SDL_MOUSEWHEEL: {
                auto& tracker = Application::the().mouse_tracker();
                auto events = tracker.notify_mouse_event(tracker.prev_buttons(), tracker.prev_x(), tracker.prev_y(), -event.wheel.y);
                auto maybe_target_window = Window::find_by_wid(event.button.windowID);
                if (!maybe_target_window) {
                    return;
                }

                for (auto& event : events) {
                    EventLoop::queue_event(*maybe_target_window, move(event));
                }
                break;
            }
            case SDL_MOUSEMOTION: {
                auto& tracker = Application::the().mouse_tracker();
                auto events = tracker.notify_mouse_event(tracker.prev_buttons(), event.motion.x, event.motion.y, 0);
                auto maybe_target_window = Window::find_by_wid(event.button.windowID);
                if (!maybe_target_window) {
                    break;
                }

                for (auto& event : events) {
                    EventLoop::queue_event(*maybe_target_window, move(event));
                }
                break;
            }
            default:
                break;
        }
    }
}

void SDLApplication::wait_for_sdl_init() {
    pthread_barrier_wait(&m_sdl_init_barrier);
}
}
#endif /* __linux__ */
