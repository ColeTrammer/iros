#include <graphics/color.h>
#include <tinput/terminal_renderer.h>
#include <tui/application.h>
#include <tui/flex_layout_engine.h>

class TestPanel final : public TUI::Panel {
    APP_OBJECT(TestPanel)

public:
    TestPanel(Color color, TextAlign alignment, String text) : m_color(color), m_alignment(alignment), m_text(move(text)) {}

    virtual Maybe<Point> cursor_position() override { return { { 0, 0 } }; }

    virtual void render() override {
        auto renderer = get_renderer();
        renderer.clear_rect(sized_rect(), m_color);

        auto style = TInput::TerminalTextStyle {
            .foreground = {},
            .background = {},
            .bold = true,
            .invert = false,
        };
        renderer.render_text(sized_rect(), m_text.view(), style, m_alignment);
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
    TextAlign m_alignment;
    String m_text;
};

int main() {
    auto app = TUI::Application::try_create();
    if (!app) {
        fprintf(stderr, "tui_test: standard input is not a terminal\n");
        return 1;
    }

    vga_color colors[9] = {
        VGA_COLOR_BLACK, VGA_COLOR_BLUE,    VGA_COLOR_BROWN, VGA_COLOR_CYAN,  VGA_COLOR_DARK_GREY,
        VGA_COLOR_GREEN, VGA_COLOR_MAGENTA, VGA_COLOR_RED,   VGA_COLOR_WHITE,
    };
    TextAlign alignments[9] = {
        TextAlign::TopLeft,     TextAlign::TopCenter,  TextAlign::TopRight,     TextAlign::CenterLeft,  TextAlign::Center,
        TextAlign::CenterRight, TextAlign::BottomLeft, TextAlign::BottomCenter, TextAlign::BottomRight,
    };

    auto& layout = app->set_layout_engine<TUI::FlexLayoutEngine>(TUI::FlexLayoutEngine::Direction::Vertical);
    for (int i = 0; i < 3; i++) {
        auto& horizontal_panel = layout.add<TUI::Panel>();
        auto& horizontal_layout = horizontal_panel.set_layout_engine<TUI::FlexLayoutEngine>(TUI::FlexLayoutEngine::Direction::Horizontal);
        for (int j = 0; j < 3; j++) {
            auto index = i * 3 + j;
            horizontal_layout.add<TestPanel>(colors[index], alignments[index], String::format("message %d", index + 1));
        }
    }

    app->set_use_mouse(true);
    app->set_use_alternate_screen_buffer(true);
    app->enter();
    return 0;
}
