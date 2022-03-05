#include <app/application.h>
#include <app/widget.h>
#include <app/window.h>
#include <graphics/renderer.h>
#include <graphics/ttf/font.h>
#include <liim/string.h>

class TestWidget final : public App::Widget {
    APP_OBJECT(TestWidget)

public:
    virtual void initialize() override {
        auto font_file = Ext::try_map_file(RESOURCE_ROOT "/usr/share/font.ttf", PROT_READ, MAP_SHARED);
        assert(font_file);

        set_font(TTF::Font::try_create_from_buffer(move(*font_file)));
        assert(font());
    }

    virtual void render() override {
        auto renderer = get_renderer();
        renderer.fill_rect(5, 5, 50, 50, ColorValue::White);
        renderer.render_text("Hello, World!", { 60, 5, sized_rect().width(), 50 }, ColorValue::White, TextAlign::TopLeft, *font());

        renderer.draw_line({ 5, 75 }, { 75, 75 }, ColorValue::White);
        renderer.draw_line({ 35, 80 }, { 35, 200 }, ColorValue::White);

        renderer.draw_line({ 5, 350 }, { 205, 200 }, ColorValue::White);
        renderer.draw_line({ 300, 50 }, { 350, 350 }, ColorValue::White);

        renderer.clear_rect({ 100, 100, 200, 200 }, Color(255, 255, 255, 123));
        App::Widget::render();
    }
};

int main() {
    auto app = App::Application::create();

    auto window = App::Window::create(nullptr, 50, 50, 400, 400, "Graphics Test", true);
    window->set_main_widget<TestWidget>();
    app->enter();
    return 0;
}
