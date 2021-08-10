#ifdef USE_SDL2
#include <app/application_sdl.h>
#include <app/window_sdl.h>

namespace App {
SDLWindow::SDLWindow(Window& window, int x, int y, int width, int height, String name, bool, WindowServer::WindowType type, wid_t)
    : m_window(window) {
    uint32_t sdl_window_flags = SDL_WINDOW_RESIZABLE;
    if (type == WindowServer::WindowType::Frameless) {
        sdl_window_flags |= SDL_WINDOW_BORDERLESS | SDL_WINDOW_HIDDEN;
    }

    offset_position(x, y);

    m_sdl_window = SDL_CreateWindow(name.string(), x, y, width, height, sdl_window_flags);
    assert(m_sdl_window);

    SDL_Window* sdl_parent = this->sdl_parent();
    if (sdl_parent) {
        SDL_SetWindowModalFor(m_sdl_window, sdl_parent);
    }

    m_window.set_id(SDL_GetWindowID(m_sdl_window));
    did_resize();
    pixels()->clear(Application::the().palette()->color(Palette::Background));
}

SDLWindow::~SDLWindow() {
    if (m_sdl_window) {
        SDL_DestroyWindow(m_sdl_window);
    }
}

SharedPtr<Bitmap> SDLWindow::pixels() {
    return m_bitmap;
}

void SDLWindow::flush_pixels() {
    assert(SDL_UpdateWindowSurface(m_sdl_window) == 0);
}

void SDLWindow::did_resize() {
    m_sdl_surface = SDL_GetWindowSurface(m_sdl_window);
    assert(m_sdl_surface);

    m_bitmap = Bitmap::wrap((uint32_t*) m_sdl_surface->pixels, m_sdl_surface->w, m_sdl_surface->h, m_window.has_alpha());

    m_window.set_rect(m_bitmap->rect());
}

void SDLWindow::do_set_visibility(int x, int y, bool visible) {
    offset_position(x, y);

    if (visible) {
        SDL_SetWindowPosition(m_sdl_window, x, y);
        SDL_ShowWindow(m_sdl_window);
        SDL_RaiseWindow(m_sdl_window);
    } else {
        SDL_HideWindow(m_sdl_window);
    }
}

SDL_Window* SDLWindow::sdl_parent() {
    if (!m_window.parent_wid()) {
        return nullptr;
    }

    auto maybe_parent = Window::find_by_wid(m_window.parent_wid());
    if (!maybe_parent) {
        return nullptr;
    }

    return static_cast<SDLWindow&>(maybe_parent.value()->platform_window()).sdl_window();
}

void SDLWindow::offset_position(int& x, int& y) {
    auto* parent = sdl_parent();
    if (!parent) {
        return;
    }

    int dx = 0;
    int dy = 0;
    SDL_GetWindowPosition(parent, &dx, &dy);
    x += dx;
    y += dy;
}
}
#endif /* USE_SDL2 */
