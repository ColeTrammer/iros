#pragma once

#include <app/object.h>
#include <liim/string.h>
#include <window_server/window.h>

namespace App {

class Window : public Object {
public:
    template<typename... Args>
    static SharedPtr<Window> create(SharedPtr<Object> parent, Args... args) {
        auto ret = SharedPtr<Window>(new Window(forward<Args>(args)...));
        if (parent) {
            parent->add_child(ret);
        }
        return ret;
    }

    virtual ~Window();

    void draw() { m_ws_window->draw(); }

    SharedPtr<PixelBuffer>& pixels() { return m_ws_window->pixels(); }

protected:
    Window(int x, int y, int width, int height, String name);

private:
    virtual bool is_window() const final { return true; }

    SharedPtr<WindowServer::Window> m_ws_window;
};

}
