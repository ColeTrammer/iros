#ifdef USE_SDL2
#include <gui/application_sdl.h>
#include <gui/window_sdl.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_hints.h>

// #define SDL_DEBUG

namespace GUI {
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
            App::EventLoop::queue_event(window.weak_from_this(), make_unique<App::WindowCloseEvent>());
            break;
        case SDL_WINDOWEVENT_EXPOSED:
            App::EventLoop::queue_event(window.weak_from_this(), make_unique<App::WindowForceRedrawEvent>());
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            App::EventLoop::queue_event(window.weak_from_this(), make_unique<App::WindowDidResizeEvent>());
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            App::EventLoop::queue_event(window.weak_from_this(), make_unique<App::WindowStateEvent>(true));
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            App::EventLoop::queue_event(window.weak_from_this(), make_unique<App::WindowStateEvent>(false));
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
            return App::MouseButton::Left;
        case SDL_BUTTON_MIDDLE:
            return App::MouseButton::Middle;
        case SDL_BUTTON_RIGHT:
            return App::MouseButton::Right;
        default:
            return 0;
    }
}

static App::Key translate_sdl_keycode(SDL_Keycode keycode) {
    switch (keycode) {
        case SDLK_RETURN:
            return App::Key::Enter;
        case SDLK_ESCAPE:
            return App::Key::Escape;
        case SDLK_BACKSPACE:
            return App::Key::Backspace;
        case SDLK_TAB:
            return App::Key::Tab;
        case SDLK_SPACE:
            return App::Key::Space;
        case SDLK_QUOTE:
            return App::Key::Quote;
        case SDLK_COMMA:
            return App::Key::Comma;
        case SDLK_MINUS:
            return App::Key::Minus;
        case SDLK_PERIOD:
            return App::Key::Period;
        case SDLK_SLASH:
            return App::Key::Slash;
        case SDLK_0:
            return App::Key::_0;
        case SDLK_1:
            return App::Key::_1;
        case SDLK_2:
            return App::Key::_2;
        case SDLK_3:
            return App::Key::_3;
        case SDLK_4:
            return App::Key::_4;
        case SDLK_5:
            return App::Key::_5;
        case SDLK_6:
            return App::Key::_6;
        case SDLK_7:
            return App::Key::_7;
        case SDLK_8:
            return App::Key::_8;
        case SDLK_9:
            return App::Key::_9;
        case SDLK_SEMICOLON:
            return App::Key::SemiColon;
        case SDLK_EQUALS:
            return App::Key::Equals;
        case SDLK_LEFTBRACKET:
            return App::Key::LeftBracket;
        case SDLK_BACKSLASH:
            return App::Key::Backslash;
        case SDLK_RIGHTBRACKET:
            return App::Key::RightBracket;
        case SDLK_BACKQUOTE:
            return App::Key::Backtick;
        case SDLK_a:
            return App::Key::A;
        case SDLK_b:
            return App::Key::B;
        case SDLK_c:
            return App::Key::C;
        case SDLK_d:
            return App::Key::D;
        case SDLK_e:
            return App::Key::E;
        case SDLK_f:
            return App::Key::F;
        case SDLK_g:
            return App::Key::G;
        case SDLK_h:
            return App::Key::H;
        case SDLK_i:
            return App::Key::I;
        case SDLK_j:
            return App::Key::J;
        case SDLK_k:
            return App::Key::K;
        case SDLK_l:
            return App::Key::L;
        case SDLK_m:
            return App::Key::M;
        case SDLK_n:
            return App::Key::N;
        case SDLK_o:
            return App::Key::O;
        case SDLK_p:
            return App::Key::P;
        case SDLK_q:
            return App::Key::Q;
        case SDLK_r:
            return App::Key::R;
        case SDLK_s:
            return App::Key::S;
        case SDLK_t:
            return App::Key::T;
        case SDLK_u:
            return App::Key::U;
        case SDLK_v:
            return App::Key::V;
        case SDLK_w:
            return App::Key::W;
        case SDLK_x:
            return App::Key::X;
        case SDLK_y:
            return App::Key::Y;
        case SDLK_z:
            return App::Key::Z;
        case SDLK_CAPSLOCK:
            return App::Key::CapsLock;
        case SDLK_F1:
            return App::Key::F1;
        case SDLK_F2:
            return App::Key::F2;
        case SDLK_F3:
            return App::Key::F3;
        case SDLK_F4:
            return App::Key::F4;
        case SDLK_F5:
            return App::Key::F5;
        case SDLK_F6:
            return App::Key::F6;
        case SDLK_F7:
            return App::Key::F7;
        case SDLK_F8:
            return App::Key::F8;
        case SDLK_F9:
            return App::Key::F9;
        case SDLK_F10:
            return App::Key::F10;
        case SDLK_F11:
            return App::Key::F11;
        case SDLK_F12:
            return App::Key::F12;
        case SDLK_SCROLLLOCK:
            return App::Key::ScrollLock;
        case SDLK_PAUSE:
            return App::Key::Pause;
        case SDLK_INSERT:
            return App::Key::Insert;
        case SDLK_HOME:
            return App::Key::Home;
        case SDLK_PAGEUP:
            return App::Key::PageUp;
        case SDLK_DELETE:
            return App::Key::Delete;
        case SDLK_END:
            return App::Key::End;
        case SDLK_PAGEDOWN:
            return App::Key::PageDown;
        case SDLK_RIGHT:
            return App::Key::RightArrow;
        case SDLK_LEFT:
            return App::Key::LeftArrow;
        case SDLK_DOWN:
            return App::Key::DownArrow;
        case SDLK_UP:
            return App::Key::UpArrow;
        case SDLK_KP_DIVIDE:
            return App::Key::Numpad_Slash;
        case SDLK_KP_MULTIPLY:
            return App::Key::Numpad_Star;
        case SDLK_KP_MINUS:
            return App::Key::Numpad_Minus;
        case SDLK_KP_PLUS:
            return App::Key::Numpad_Plus;
        case SDLK_KP_ENTER:
            return App::Key::Numpad_Enter;
        case SDLK_KP_1:
            return App::Key::Numpad_1;
        case SDLK_KP_2:
            return App::Key::Numpad_2;
        case SDLK_KP_3:
            return App::Key::Numpad_3;
        case SDLK_KP_4:
            return App::Key::Numpad_4;
        case SDLK_KP_5:
            return App::Key::Numpad_5;
        case SDLK_KP_6:
            return App::Key::Numpad_6;
        case SDLK_KP_7:
            return App::Key::Numpad_7;
        case SDLK_KP_8:
            return App::Key::Numpad_8;
        case SDLK_KP_9:
            return App::Key::Numpad_9;
        case SDLK_KP_0:
            return App::Key::Numpad_0;
        case SDLK_KP_PERIOD:
            return App::Key::Numpad_Period;
        case SDLK_APPLICATION:
            return App::Key::Application;
        case SDLK_POWER:
            return App::Key::Power;
        case SDLK_LCTRL:
            return App::Key::LeftControl;
        case SDLK_LSHIFT:
            return App::Key::LeftShift;
        case SDLK_LALT:
            return App::Key::LeftAlt;
        case SDLK_LGUI:
            return App::Key::LeftMeta;
        case SDLK_RCTRL:
            return App::Key::RightControl;
        case SDLK_RSHIFT:
            return App::Key::RightShift;
        case SDLK_RALT:
            return App::Key::RightAlt;
        case SDLK_RGUI:
            return App::Key::RightMeta;
        default:
            return App::Key::None;
    }
}

static int translate_sdl_modifiers(uint16_t sdl_modifers) {
    int modifiers = 0;
    if (sdl_modifers & KMOD_SHIFT) {
        modifiers |= App::KeyModifier::Shift;
    }
    if (sdl_modifers & KMOD_ALT) {
        modifiers |= App::KeyModifier::Alt;
    }
    if (sdl_modifers & KMOD_CTRL) {
        modifiers |= App::KeyModifier::Control;
    }
    if (sdl_modifers & KMOD_GUI) {
        modifiers |= App::KeyModifier::Meta;
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
                App::EventLoop::queue_event(
                    *maybe_window, input_tracker().notify_key_event(translate_sdl_keycode(event.key.keysym.sym), modifiers, false, true));
                break;
            }
            case SDL_TEXTINPUT: {
                auto maybe_window = Window::find_by_wid(event.text.windowID);
                if (!maybe_window) {
                    break;
                }

                App::EventLoop::queue_event(*maybe_window, make_unique<App::TextEvent>(event.text.text));
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
                    App::EventLoop::queue_event(*maybe_target_window, move(event));
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
                    App::EventLoop::queue_event(*maybe_target_window, move(event));
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
                    App::EventLoop::queue_event(*maybe_target_window, move(event));
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
