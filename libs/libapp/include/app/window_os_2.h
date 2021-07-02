#pragma once

#include <app/window.h>

namespace App {
class OSWindow final : public PlatformWindow {
public:
    OSWindow(Window& window, int x, int y, int width, int height, String name, bool has_alpha, WindowServer::WindowType window_type,
             wid_t parent_id);
    virtual ~OSWindow() override;

    virtual SharedPtr<Bitmap> pixels() override { return m_back_buffer; }
    virtual void flush_pixels() override;
    virtual void did_resize() override;
    virtual void do_set_visibility(int x, int y, bool visible) override;

private:
    void do_resize(int width, int height);

    Window& m_window;
    String m_shm_path;
    void* m_raw_pixels;
    size_t m_raw_pixels_size { 0 };
    SharedPtr<Bitmap> m_front_buffer;
    SharedPtr<Bitmap> m_back_buffer;
};
}
