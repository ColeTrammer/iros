#ifdef USE_SDL2
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

static Key translate_sdl_keycode(SDL_Keycode keycode) {
    switch (keycode) {
        case SDLK_RETURN:
            return Key::Enter;
        case SDLK_ESCAPE:
            return Key::Escape;
        case SDLK_BACKSPACE:
            return Key::Backspace;
        case SDLK_TAB:
            return Key::Tab;
        case SDLK_SPACE:
            return Key::Space;
        case SDLK_QUOTE:
            return Key::Quote;
        case SDLK_COMMA:
            return Key::Comma;
        case SDLK_MINUS:
            return Key::Minus;
        case SDLK_PERIOD:
            return Key::Period;
        case SDLK_SLASH:
            return Key::Slash;
        case SDLK_0:
            return Key::_0;
        case SDLK_1:
            return Key::_1;
        case SDLK_2:
            return Key::_2;
        case SDLK_3:
            return Key::_3;
        case SDLK_4:
            return Key::_4;
        case SDLK_5:
            return Key::_5;
        case SDLK_6:
            return Key::_6;
        case SDLK_7:
            return Key::_7;
        case SDLK_8:
            return Key::_8;
        case SDLK_9:
            return Key::_9;
        case SDLK_SEMICOLON:
            return Key::SemiColon;
        case SDLK_EQUALS:
            return Key::Equals;
        case SDLK_LEFTBRACKET:
            return Key::LeftBracket;
        case SDLK_BACKSLASH:
            return Key::Backslash;
        case SDLK_RIGHTBRACKET:
            return Key::RightBracket;
        case SDLK_BACKQUOTE:
            return Key::Backtick;
        case SDLK_a:
            return Key::A;
        case SDLK_b:
            return Key::B;
        case SDLK_c:
            return Key::C;
        case SDLK_d:
            return Key::D;
        case SDLK_e:
            return Key::E;
        case SDLK_f:
            return Key::F;
        case SDLK_g:
            return Key::G;
        case SDLK_h:
            return Key::H;
        case SDLK_i:
            return Key::I;
        case SDLK_j:
            return Key::J;
        case SDLK_k:
            return Key::K;
        case SDLK_l:
            return Key::L;
        case SDLK_m:
            return Key::M;
        case SDLK_n:
            return Key::N;
        case SDLK_o:
            return Key::O;
        case SDLK_p:
            return Key::P;
        case SDLK_q:
            return Key::Q;
        case SDLK_r:
            return Key::R;
        case SDLK_s:
            return Key::S;
        case SDLK_t:
            return Key::T;
        case SDLK_u:
            return Key::U;
        case SDLK_v:
            return Key::V;
        case SDLK_w:
            return Key::W;
        case SDLK_x:
            return Key::X;
        case SDLK_y:
            return Key::Y;
        case SDLK_z:
            return Key::Z;
        case SDLK_CAPSLOCK:
            return Key::CapsLock;
        case SDLK_F1:
            return Key::F1;
        case SDLK_F2:
            return Key::F2;
        case SDLK_F3:
            return Key::F3;
        case SDLK_F4:
            return Key::F4;
        case SDLK_F5:
            return Key::F5;
        case SDLK_F6:
            return Key::F6;
        case SDLK_F7:
            return Key::F7;
        case SDLK_F8:
            return Key::F8;
        case SDLK_F9:
            return Key::F9;
        case SDLK_F10:
            return Key::F10;
        case SDLK_F11:
            return Key::F11;
        case SDLK_F12:
            return Key::F12;
        case SDLK_SCROLLLOCK:
            return Key::ScrollLock;
        case SDLK_PAUSE:
            return Key::Pause;
        case SDLK_INSERT:
            return Key::Insert;
        case SDLK_HOME:
            return Key::Home;
        case SDLK_PAGEUP:
            return Key::PageUp;
        case SDLK_DELETE:
            return Key::Delete;
        case SDLK_END:
            return Key::End;
        case SDLK_PAGEDOWN:
            return Key::PageDown;
        case SDLK_RIGHT:
            return Key::RightArrow;
        case SDLK_LEFT:
            return Key::LeftArrow;
        case SDLK_DOWN:
            return Key::DownArrow;
        case SDLK_UP:
            return Key::UpArrow;
        case SDLK_KP_DIVIDE:
            return Key::Numpad_Slash;
        case SDLK_KP_MULTIPLY:
            return Key::Numpad_Star;
        case SDLK_KP_MINUS:
            return Key::Numpad_Minus;
        case SDLK_KP_PLUS:
            return Key::Numpad_Plus;
        case SDLK_KP_ENTER:
            return Key::Numpad_Enter;
        case SDLK_KP_1:
            return Key::Numpad_1;
        case SDLK_KP_2:
            return Key::Numpad_2;
        case SDLK_KP_3:
            return Key::Numpad_3;
        case SDLK_KP_4:
            return Key::Numpad_4;
        case SDLK_KP_5:
            return Key::Numpad_5;
        case SDLK_KP_6:
            return Key::Numpad_6;
        case SDLK_KP_7:
            return Key::Numpad_7;
        case SDLK_KP_8:
            return Key::Numpad_8;
        case SDLK_KP_9:
            return Key::Numpad_9;
        case SDLK_KP_0:
            return Key::Numpad_0;
        case SDLK_KP_PERIOD:
            return Key::Numpad_Period;
        case SDLK_APPLICATION:
            return Key::Application;
        case SDLK_POWER:
            return Key::Power;
        case SDLK_LCTRL:
            return Key::LeftControl;
        case SDLK_LSHIFT:
            return Key::LeftShift;
        case SDLK_LALT:
            return Key::LeftAlt;
        case SDLK_LGUI:
            return Key::LeftMeta;
        case SDLK_RCTRL:
            return Key::RightControl;
        case SDLK_RSHIFT:
            return Key::RightShift;
        case SDLK_RALT:
            return Key::RightAlt;
        case SDLK_RGUI:
            return Key::RightMeta;
        default:
            return Key::None;
    }
}

static int translate_sdl_modifiers(uint16_t sdl_modifers) {
    int modifiers = 0;
    if (sdl_modifers & KMOD_SHIFT) {
        modifiers |= KeyModifier::Shift;
    }
    if (sdl_modifers & KMOD_ALT) {
        modifiers |= KeyModifier::Alt;
    }
    if (sdl_modifers & KMOD_CTRL) {
        modifiers |= KeyModifier::Control;
    }
    if (sdl_modifers & KMOD_GUI) {
        modifiers |= KeyModifier::Meta;
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

                auto modifiers = translate_sdl_modifiers(event.key.keysym.mod);
                EventLoop::queue_event(
                    *maybe_window, make_unique<KeyEvent>(KeyEventType::Down, "", translate_sdl_keycode(event.key.keysym.sym), modifiers));
                break;
            }
            case SDL_TEXTINPUT: {
                auto maybe_window = Window::find_by_wid(event.text.windowID);
                if (!maybe_window) {
                    break;
                }

                auto modifiers = translate_sdl_modifiers(SDL_GetModState());
                EventLoop::queue_event(*maybe_window, make_unique<KeyEvent>(KeyEventType::Down, event.text.text, Key::None, modifiers));
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                auto button = translate_sdl_button(event.button.button);
                if (!button) {
                    break;
                }

                auto buttons_down = Application::the().input_tracker().prev_buttons();
                if (event.button.state == SDL_PRESSED) {
                    buttons_down |= button;
                } else {
                    buttons_down &= ~button;
                }

                auto events = Application::the().input_tracker().notify_mouse_event(buttons_down, event.button.x, event.button.y, 0,
                                                                                    translate_sdl_modifiers(SDL_GetModState()));
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
                auto& tracker = Application::the().input_tracker();
                auto events = tracker.notify_mouse_event(tracker.prev_buttons(), tracker.prev_x(), tracker.prev_y(), -event.wheel.y,
                                                         translate_sdl_modifiers(SDL_GetModState()));
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
                auto& tracker = Application::the().input_tracker();
                auto events = tracker.notify_mouse_event(tracker.prev_buttons(), event.motion.x, event.motion.y, 0,
                                                         translate_sdl_modifiers(SDL_GetModState()));
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
#endif /* USE_SDL2 */
