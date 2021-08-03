#include <graphics/color.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>
#include <tui/flex_layout_engine.h>

class TestPanel final : public TUI::Panel {
    APP_OBJECT(TestPanel)

public:
    TestPanel(Color color) : m_color(color) {}

    virtual Maybe<Point> cursor_position() override { return { { 0, 0 } }; }

    virtual void render() override {
        auto renderer = get_renderer();
        renderer.clear_rect(sized_rect(), m_color);
    }

    virtual void on_mouse_down(const App::MouseEvent& event) override {
        m_color = m_color.invert();
        invalidate();
        Panel::on_mouse_down(event);
    }

    virtual void on_key_event(const App::KeyEvent& event) override {
        m_color = m_color.invert();
        invalidate();
        Panel::on_key_event(event);
    }

private:
    Color m_color;
};

int main() {
    auto app = TUI::Application::try_create();
    if (!app) {
        fprintf(stderr, "tui_test: standard input is not a terminal\n");
        return 1;
    }

    auto& layout = app->set_layout_engine<TUI::FlexLayoutEngine>(TUI::FlexLayoutEngine::Direction::Vertical);
    layout.add<TestPanel>(VGA_COLOR_RED);

    auto& horizontal_panel = layout.add<TUI::Panel>();
    auto& horizontal_layout = horizontal_panel.set_layout_engine<TUI::FlexLayoutEngine>(TUI::FlexLayoutEngine::Direction::Horizontal);
    horizontal_layout.add<TestPanel>(VGA_COLOR_CYAN);
    horizontal_layout.add<TestPanel>(VGA_COLOR_GREEN);

    app->set_use_mouse(true);
    app->set_use_alternate_screen_buffer(true);
    app->enter();
    return 0;
}