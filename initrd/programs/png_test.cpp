#include <app/app.h>
#include <app/window.h>
#include <graphics/png.h>
#include <graphics/renderer.h>
#include <liim/string.h>

static SharedPtr<PixelBuffer> s_bitmap;

class TestWindow final : public App::Window {
    APP_OBJECT(TestWindow)

public:
    using App::Window::Window;

    virtual void render() override {
        Renderer renderer(*pixels());

        auto w = s_bitmap->width();
        auto h = s_bitmap->height();
        auto rect = Rect { 0, 0, w, h };
        renderer.draw_bitmap(*s_bitmap, rect, rect);

        App::Window::render();
    }
};

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;

#ifdef __os_2__
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <png>\n", *argv);
        return 1;
    }

    s_bitmap = decode_png_file(argv[1]);
    if (!s_bitmap) {
        fprintf(stderr, "Failed to decode PNG\n");
        return 1;
    }

    App::App app;

    auto window = TestWindow::create(nullptr, 50, 50, 400, 400, "Graphics Test");
    window->draw();
    app.enter();
    return 0;
#endif /* __os_2__ */
}
