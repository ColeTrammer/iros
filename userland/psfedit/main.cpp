#include <app/app.h>
#include <app/box_layout.h>
#include <app/button.h>
#include <app/event.h>
#include <app/text_label.h>
#include <app/widget.h>
#include <app/window.h>
#include <graphics/font.h>
#include <graphics/renderer.h>

class GlyphEditorWidgetCell final : public App::Widget {
    APP_OBJECT(GlyphEditorWidgetCell)

public:
    virtual void render() {
        Renderer renderer(*window()->pixels());
        Color color(255, 255, 255);
        if (m_bitmap && m_bitmap->get(m_index)) {
            color = Color(0, 0, 0);
        }

        renderer.set_color(color);
        renderer.fill_rect(rect());

        renderer.set_color(Color(0, 0, 0));
        renderer.draw_rect(rect());
    }

    virtual void on_mouse_event(App::MouseEvent& event) override {
        if (m_bitmap && event.left() == MOUSE_DOWN) {
            m_bitmap->flip(m_index);
            window()->draw();
        }

        App::Widget::on_mouse_event(event);
    }

private:
    GlyphEditorWidgetCell(SharedPtr<Bitmap<uint8_t>>& bitmap, int index) : m_bitmap(bitmap), m_index(index) {}

    SharedPtr<Bitmap<uint8_t>>& m_bitmap;
    int m_index { 0 };
};

class GlyphEditorWidget final : public App::Widget {
    APP_OBJECT(GlyphEditorWidget)

public:
    void set_bitmap(SharedPtr<Bitmap<uint8_t>> bitmap) { m_bitmap = move(bitmap); }

private:
    GlyphEditorWidget(int width, int height) {
        auto& layout = set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Horizontal);
        auto& left_container = layout.add<App::Widget>();

        auto& row_layout = left_container.set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Vertical);
        row_layout.set_spacing(0);
        for (int i = 0; i < height; i++) {
            auto& row_widget = row_layout.add<App::Widget>();
            auto& col_layout = row_widget.set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Horizontal);
            col_layout.set_margins({ 0, 0, 0, 0 });
            col_layout.set_spacing(0);

            for (int j = 0; j < width; j++) {
                int index = i * width + width - j - 1;
                col_layout.add<GlyphEditorWidgetCell>(m_bitmap, index);
            }
        }

        layout.add<App::TextLabel>("Important Text");
    }

    SharedPtr<Bitmap<uint8_t>> m_bitmap;
};

int main() {
    App::App app;

    auto window = App::Window::create(nullptr, 250, 150, 500, 600, "PSF Edit");

    auto& font = Font::default_font();

    auto& layout = window->set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Vertical);
    auto& glyph_editor = layout.add<GlyphEditorWidget>(8, 16);
    glyph_editor.set_bitmap(font.get_for_character(0));

    auto& glyph_widget = layout.add<App::Widget>();
    auto& row_layout = glyph_widget.set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Vertical);
    row_layout.set_spacing(0);

    for (int i = 0; i < 16; i++) {
        auto& row_widget = row_layout.add<App::Widget>();
        auto& col_layout = row_widget.set_layout<App::BoxLayout>(App::BoxLayout::Orientation::Horizontal);
        col_layout.set_margins({ 0, 0, 0, 0 });
        col_layout.set_spacing(0);
        for (int j = 0; j < 16; j++) {
            int code_point = i * 16 + j;
            auto& button = col_layout.add<App::Button>(String(static_cast<char>(code_point)));
            button.on_click = [&, code_point]() {
                glyph_editor.set_bitmap(font.get_for_character(code_point));
                window->draw();
            };
        }
    }

    window->draw();
    app.enter();
    return 0;
}
