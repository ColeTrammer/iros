#include <app/application.h>
#include <app/widget.h>
#include <app/window.h>
#include <graphics/png.h>
#include <graphics/renderer.h>
#include <liim/string.h>

static SharedPtr<Bitmap> s_bitmap;

class TestWidget final : public App::Widget {
    APP_WIDGET(App::Widget, TestWidget)

public:
    TestWidget() {}

    virtual void render() override {
        auto renderer = get_renderer();
        renderer.fill_rect(sized_rect(), ColorValue::Black);

        auto w = s_bitmap->width();
        auto h = s_bitmap->height();
        auto rect = Rect { 0, 0, w, h };
        renderer.draw_bitmap(*s_bitmap, rect, rect);

        App::Widget::render();
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

    auto app = App::Application::create();

    auto window = App::Window::create(nullptr, 50, 50, 400, 400, "Graphics Test");
    window->set_main_widget<TestWidget>();
    app->enter();
    return 0;
#endif /* __os_2__ */
}
