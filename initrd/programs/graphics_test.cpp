#include <app/app.h>
#include <app/widget.h>
#include <app/window.h>
#include <graphics/renderer.h>
#include <liim/string.h>

class TestWidget final : public App::Widget {
    APP_OBJECT(TestWidget)

public:
    virtual void render() override {
        Renderer renderer(*window()->pixels());
        renderer.fill_rect(5, 5, 50, 50, ColorValue::White);
        renderer.render_text(60, 5, "Hello, World!", ColorValue::White);

        renderer.draw_line({ 5, 75 }, { 75, 75 }, ColorValue::White);
        renderer.draw_line({ 35, 80 }, { 35, 200 }, ColorValue::White);

        renderer.draw_line({ 5, 350 }, { 205, 200 }, ColorValue::White);
        renderer.draw_line({ 300, 50 }, { 350, 350 }, ColorValue::White);

        App::Widget::render();
    }
};

int main() {
#ifdef __os_2__
    App::App app;

    auto window = App::Window::create(nullptr, 50, 50, 400, 400, "Graphics Test");
    window->set_main_widget<TestWidget>();
    app.enter();
    return 0;
#endif /* __os_2__ */
}
