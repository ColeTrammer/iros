#pragma once

#include <app/application.h>
#include <pthread.h>

#include <SDL2/SDL_events.h>

namespace App {
class SDLApplication final : public Application {
    APP_OBJECT(SDLApplication)

public:
    static SDLApplication& the();
    SDLApplication();
    virtual ~SDLApplication() override;

    void run_sdl();

    virtual UniquePtr<PlatformWindow> create_window(Window& window, int x, int y, int width, int height, String name, bool has_alpha,
                                                    WindowServer::WindowType type, wid_t parent_id) override;

private:
    void wait_for_sdl_init();

    void on_sdl_window_event(const SDL_Event& event);

    virtual bool is_sdl_application() const { return true; }

    pthread_t m_sdl_thread;
    pthread_barrier_t m_sdl_init_barrier;
};
}
