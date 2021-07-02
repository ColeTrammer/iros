#include <app/application_sdl.h>
#include <unistd.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>

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
    initialize_palette(Palette::create_default());

    pthread_barrier_init(&m_sdl_init_barrier, nullptr, 2);
    pthread_create(&m_sdl_thread, nullptr, thread_run_sdl, nullptr);
    wait_for_sdl_init();
}

SDLApplication::~SDLApplication() {}

void SDLApplication::run_sdl() {
    SDL_SetMainReady();
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);

    pthread_barrier_wait(&m_sdl_init_barrier);

    for (;;) {
        SDL_Event event;
        SDL_WaitEvent(&event);

        switch (event.type) {
            default:
                break;
        }
    }
}

void SDLApplication::wait_for_sdl_init() {
    pthread_barrier_wait(&m_sdl_init_barrier);
}
}
