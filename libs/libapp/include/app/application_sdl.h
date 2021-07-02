#pragma once

#include <app/application.h>

namespace App {
class SDLApplication final : public Application {
public:
    SDLApplication();
    virtual ~SDLApplication() override;

private:
    virtual bool is_sdl_application() const { return true; }
};
}
