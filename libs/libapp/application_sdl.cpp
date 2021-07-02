#ifdef __linux__
#include <app/application_sdl.h>
#include <app/window_sdl.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>

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
        case SDL_WINDOWEVENT_CLOSE:
            EventLoop::queue_event(window.weak_from_this(), make_unique<WindowEvent>(WindowEvent::Type::Close));
            break;
        case SDL_WINDOWEVENT_EXPOSED:
            EventLoop::queue_event(window.weak_from_this(), make_unique<WindowEvent>(WindowEvent::Type::ForceRedraw));
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            EventLoop::queue_event(window.weak_from_this(), make_unique<WindowEvent>(WindowEvent::Type::DidResize));
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

void SDLApplication::run_sdl() {
    SDL_SetMainReady();
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);

    pthread_barrier_wait(&m_sdl_init_barrier);

    for (;;) {
        SDL_Event event;
        SDL_WaitEvent(&event);

        switch (event.type) {
            case SDL_WINDOWEVENT:
                on_sdl_window_event(event);
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                auto button = translate_sdl_button(event.button.button);
                if (!button) {
                    return;
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
                    return;
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
                    return;
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
