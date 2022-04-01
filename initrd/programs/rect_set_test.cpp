#include <graphics/rect_set.h>
#include <graphics/renderer.h>
#include <gui/application.h>
#include <gui/widget.h>
#include <gui/window.h>

class TestWidget final : public GUI::Widget {
    APP_WIDGET(GUI::Widget, TestWidget)

public:
    TestWidget() {}

    virtual void render() override {
        auto renderer = get_renderer();

        RectSet s;
        s.add({ 50, 50, 300, 300 });
        s.add({ 40, 60, 100, 100 });
        s.add({ 300, 300, 100, 100 });
        s.add({ 25, 25, 450, 450 });
        s.add({ 450, 450, 50, 125 });
        s.add({ 425, 425, 100, 100 });
        s.add({ 550, 550, 25, 25 });
        s.subtract({ 50, 450, 480, 10 });
        s.subtract({ 40, 40, 50, 50 });

        for (auto& r : s) {
            renderer.fill_rect(r, ColorValue::White);
            renderer.draw_rect(r, ColorValue::Black);
            fprintf(stderr, "x=%d y=%d w=%d h=%d\n", r.x(), r.y(), r.width(), r.height());
        }

        GUI::Widget::render();
    }
};

int main() {
    auto app = GUI::Application::create();

    auto window = GUI::Window::create(nullptr, 50, 50, 600, 600, "Rect Set Test");
    window->set_main_widget<TestWidget>();
    app->enter();
    return 0;
}
