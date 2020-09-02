#include <app/app.h>
#include <app/widget.h>
#include <app/window.h>
#include <graphics/rect_set.h>
#include <graphics/renderer.h>

class TestWidget final : public App::Widget {
    APP_OBJECT(TestWidget)

public:
    virtual void render() override {
        Renderer renderer(*window()->pixels());

        RectSet s;
        s.add({ 50, 50, 300, 300 });
        s.add({ 40, 60, 100, 100 });
        s.add({ 300, 300, 100, 100 });
        s.add({ 25, 25, 450, 450 });
        s.add({ 450, 450, 50, 125 });
        s.add({ 425, 425, 100, 100 });
        s.add({ 550, 550, 25, 25 });

        for (auto& r : s) {
            renderer.fill_rect(r, ColorValue::White);
            renderer.draw_rect(r, ColorValue::Black);
            fprintf(stderr, "x=%d y=%d w=%d h=%d\n", r.x(), r.y(), r.width(), r.height());
        }

        App::Widget::render();
    }
};

int main() {
#ifdef __os_2__
    App::App app;

    auto window = App::Window::create(nullptr, 50, 50, 600, 600, "Rect Set Test");
    window->set_main_widget<TestWidget>();
    app.enter();
    return 0;
#endif /* __os_2__ */
}
