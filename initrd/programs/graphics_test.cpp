#include <app/app.h>
#include <app/window.h>
#include <graphics/renderer.h>
#include <liim/string.h>

class TestWindow final : public App::Window {
    APP_OBJECT(TestWindow)

public:
    using App::Window::Window;

    virtual void render() override {
        Renderer renderer(*pixels());
        renderer.fill_rect(5, 5, 50, 50, ColorValue::White);
        renderer.render_text(60, 5, "Hello, World!", ColorValue::White);

        renderer.draw_line({ 5, 75 }, { 75, 75 }, ColorValue::White);
        renderer.draw_line({ 35, 80 }, { 35, 200 }, ColorValue::White);

        // renderer.draw_line({ 5, 350 }, { 205, 200 }, ColorValue::White);

        App::Window::render();
    }
};

int main() {
    App::App app;

    auto window = TestWindow::create(nullptr, 50, 50, 400, 400, "Graphics Test");
    window->draw();
    app.enter();
    return 0;
}
