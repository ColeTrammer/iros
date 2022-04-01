#pragma once

#include <app/application.h>
#include <app/forward.h>
#include <eventloop/event_loop.h>
#include <eventloop/input_tracker.h>
#include <graphics/palette.h>
#include <gui/forward.h>
#include <liim/pointers.h>
#include <window_server/message.h>

namespace GUI {
class WindowServerListener {
public:
    virtual ~WindowServerListener() {}

    virtual void server_did_create_window(const WindowServer::Server::ServerDidCreatedWindow&) {}
    virtual void server_did_change_window_title(const WindowServer::Server::ServerDidChangeWindowTitle&) {}
    virtual void server_did_close_window(const WindowServer::Server::ServerDidCloseWindow&) {}
    virtual void server_did_make_window_active(const WindowServer::Server::ServerDidMakeWindowActive&) {}
};

class Application : public App::Application {
public:
    static SharedPtr<Application> create();
    static Application& the();

    virtual ~Application();

    virtual App::Margins default_margins() const override;
    virtual int default_spacing() const override;

    App::InputTracker& input_tracker() { return m_mouse_tracker; }

    SharedPtr<Palette> palette() const { return m_palette; }

    virtual void set_window_server_listener(WindowServerListener&) {}
    virtual void remove_window_server_listener() {}

    virtual UniquePtr<PlatformWindow> create_window(Window& window, int x, int y, int width, int height, String name, bool has_alpha,
                                                    WindowServer::WindowType type, wid_t parent_id) = 0;
    virtual void set_active_window(wid_t) {}
    virtual void set_global_palette(const String& path);

    virtual bool is_os_application() const { return false; }
    virtual bool is_sdl_application() const { return false; }

protected:
    Application();

    void initialize_palette(SharedPtr<Palette> palette) { m_palette = move(palette); }

    virtual void before_enter() override;

private:
    friend class WindowServerClient;

    App::InputTracker m_mouse_tracker;
    SharedPtr<Palette> m_palette;
};
}
