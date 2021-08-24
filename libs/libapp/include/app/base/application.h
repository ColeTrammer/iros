#pragma once

#include <eventloop/event_loop.h>
#include <eventloop/object.h>

namespace App::Base {
class Application : public App::Object {
    APP_OBJECT(Application)

public:
    static Application& the();

    EventLoop& main_event_loop() { return m_loop; }
    void enter();

protected:
    Application();

private:
    EventLoop m_loop;
};
}
