#pragma once

#include <app/application.h>
#include <pthread.h>

namespace App {
class SDLApplication final : public Application {
public:
    static SDLApplication& the();
    SDLApplication();
    virtual ~SDLApplication() override;

    void run_sdl();
    void wait_for_sdl_init();

private:
    virtual bool is_sdl_application() const { return true; }

    pthread_t m_sdl_thread;
    pthread_barrier_t m_sdl_init_barrier;
};
}
