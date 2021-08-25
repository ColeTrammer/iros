#pragma once

#include <eventloop/event_loop.h>
#include <eventloop/object.h>

namespace App::Base {
class Application : public App::Object {
    APP_OBJECT(Application)

public:
    static Application& the();

    virtual ~Application() override;

    virtual Margins default_margins() const = 0;
    virtual int default_spacing() const = 0;

    EventLoop& main_event_loop() { return m_loop; }
    void enter();

protected:
    Application();

    virtual void before_enter() {}

private:
    EventLoop m_loop;
};
}
