#pragma once

#include <app/object.h>
#include <liim/maybe.h>
#include <liim/string.h>
#include <window_server/window.h>

namespace App {

class Window : public Object {
public:
    template<typename... Args>
    static SharedPtr<Window> create(SharedPtr<Object> parent, Args&&... args) {
        auto ret = SharedPtr<Window>(new Window(forward<Args>(args)...));
        register_window(ret);
        if (parent) {
            parent->add_child(ret);
        }
        return ret;
    }

    static Maybe<WeakPtr<Window>> find_by_wid(wid_t wid);

    virtual ~Window();

    void draw() { m_ws_window->draw(); }

    SharedPtr<PixelBuffer>& pixels() { return m_ws_window->pixels(); }

    wid_t wid() const { return m_ws_window->wid(); }

protected:
    Window(int x, int y, int width, int height, String name);

private:
    virtual bool is_window() const final { return true; }

    virtual void on_event(Event& event) override;

    static void register_window(const SharedPtr<Window>& window);
    static void unregister_window(wid_t wid);

    SharedPtr<WindowServer::Window> m_ws_window;
};

}
