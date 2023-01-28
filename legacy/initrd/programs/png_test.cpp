#include <graphics/png.h>
#include <graphics/renderer.h>
#include <gui/application.h>
#include <gui/widget.h>
#include <gui/window.h>
#include <liim/string.h>

static SharedPtr<Bitmap> s_bitmap;

class TestWidget final : public GUI::Widget {
    APP_WIDGET(GUI::Widget, TestWidget)

public:
    TestWidget() {}

    virtual void render() override {
        auto renderer = get_renderer();
        renderer.fill_rect(sized_rect(), ColorValue::Black);

        auto w = s_bitmap->width();
        auto h = s_bitmap->height();
        auto rect = Rect { 0, 0, w, h };
        renderer.draw_bitmap(*s_bitmap, rect, rect);

        GUI::Widget::render();
    }
};

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;

#ifdef __iros__
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <png>\n", *argv);
        return 1;
    }

    s_bitmap = decode_png_file(argv[1]);
    if (!s_bitmap) {
        fprintf(stderr, "Failed to decode PNG\n");
        return 1;
    }

    auto app = GUI::Application::create();

    auto window = GUI::Window::create(nullptr, 50, 50, 400, 400, "Graphics Test");
    window->set_main_widget<TestWidget>();
    app->enter();
    return 0;
#endif /* __iros__ */
}
