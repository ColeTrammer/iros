#include <app/app.h>
#include <app/window.h>
#include <graphics/rect_set.h>
#include <graphics/renderer.h>

class TestWindow final : public App::Window {
    APP_OBJECT(TestWindow)

public:
    using App::Window::Window;

    virtual void render() override {
        Renderer renderer(*pixels());

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

        App::Window::render();
    }
};

int main() {
#ifdef __os_2__
    App::App app;

    auto window = TestWindow::create(nullptr, 50, 50, 600, 600, "Rect Set Test");
    window->draw();
    app.enter();
    return 0;
#endif /* __os_2__ */
}
