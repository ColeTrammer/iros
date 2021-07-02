#ifdef __linux__
#include <app/application_sdl.h>
#include <app/window_sdl.h>

namespace App {
SDLWindow::SDLWindow(Window& window, int x, int y, int width, int height, String name, bool, WindowServer::WindowType, wid_t)
    : m_window(window) {
    uint32_t sdl_window_flags = SDL_WINDOW_RESIZABLE;
    m_sdl_window = SDL_CreateWindow(name.string(), x, y, width, height, sdl_window_flags);
    assert(m_sdl_window);

    m_window.set_id(SDL_GetWindowID(m_sdl_window));
    did_resize();
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
    SDL_SetWindowPosition(m_sdl_window, x, y);
    if (visible) {
        SDL_ShowWindow(m_sdl_window);
    } else {
        SDL_HideWindow(m_sdl_window);
    }
}
}
#endif /* __linux__ */
