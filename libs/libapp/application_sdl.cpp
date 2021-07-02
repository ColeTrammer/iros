#include <app/application_sdl.h>

namespace App {
SDLApplication::SDLApplication() {
    initialize_palette(Palette::create_default());
}

SDLApplication::~SDLApplication() {}
}
