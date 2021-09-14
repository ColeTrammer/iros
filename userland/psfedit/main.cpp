#include <app/application.h>
#include <app/button.h>
#include <app/flex_layout_engine.h>
#include <app/text_label.h>
#include <app/widget.h>
#include <app/window.h>
#include <eventloop/event.h>
#include <graphics/font.h>
#include <graphics/psf/font.h>
#include <graphics/renderer.h>
#include <stdlib.h>
#include <unistd.h>

class GlyphEditorWidgetCell final : public App::Widget {
    APP_OBJECT(GlyphEditorWidgetCell)

public:
    virtual void initialize() override {
        on<App::MouseDownEvent>([this](const App::MouseDownEvent& event) {
            if (m_bitset && event.left_button()) {
                m_bitset->flip(m_index);
                invalidate();
                return true;
            }
            return false;
        });

        App::Widget::initialize();
    }

    virtual void render() {
        auto renderer = get_renderer();
        auto c = ColorValue::White;
        if (m_bitset && m_bitset->get(m_index)) {
            c = ColorValue::Black;
        }

        renderer.fill_rect(sized_rect(), c);

        renderer.draw_rect(sized_rect(), ColorValue::Black);
    }

private:
    GlyphEditorWidgetCell(Bitset<uint8_t>*& bitset, int index) : m_bitset(bitset), m_index(index) {}

    Bitset<uint8_t>*& m_bitset;
    int m_index { 0 };
};

class GlyphEditorWidget final : public App::Widget {
    APP_OBJECT(GlyphEditorWidget)

public:
    void set_bitset(Bitset<uint8_t>* bitset, char c) {
        m_bitset = bitset;
        m_info_label->set_text(String::format("Editing glyph %d (%c)", c, c));
        invalidate();
    }

private:
    GlyphEditorWidget(int width, int height, SharedPtr<Font> font) : m_width(width), m_height(height) { set_font(font); }
    virtual void initialize() override {
        auto& layout = set_layout_engine<App::HorizontalFlexLayoutEngine>();
        auto& left_container = layout.add<App::Widget>();

        auto& row_layout = left_container.set_layout_engine<App::VerticalFlexLayoutEngine>();
        for (int i = 0; i < m_height; i++) {
            auto& row_widget = row_layout.add<App::Widget>();
            auto& col_layout = row_widget.set_layout_engine<App::HorizontalFlexLayoutEngine>();

            for (int j = 0; j < m_width; j++) {
                int index = i * m_width + m_width - j - 1;
                col_layout.add<GlyphEditorWidgetCell>(m_bitset, index);
            }
        }

        auto& text_container = layout.add<App::Widget>();
        auto& text_layout = text_container.set_layout_engine<App::VerticalFlexLayoutEngine>();

        m_info_label = text_layout.add<App::TextLabel>("").shared_from_this();

        auto& demo_label = text_layout.add<App::TextLabel>("abcdefghijklmnopqrstuvwxyz\n"
                                                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
                                                           "1234567890`~!@#$%^&*()-=_+\n"
                                                           "[]{}\\|;:'\",.<>/?");
        demo_label.set_font(font());

        App::Widget::initialize();
    }

    Bitset<uint8_t>* m_bitset { nullptr };
    int m_width { 0 };
    int m_height { 0 };
    SharedPtr<App::TextLabel> m_info_label;
};

static void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s [-c] [-o save-destination] [psf-font-file]\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    bool create_new_font = false;
    char* save_destination = nullptr;

    int opt;
    while ((opt = getopt(argc, argv, ":co:")) != -1) {
        switch (opt) {
            case 'c':
                create_new_font = true;
                break;
            case 'o':
                save_destination = optarg;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if ((create_new_font && (!save_destination || optind != argc)) || optind + 1 < argc) {
        print_usage_and_exit(*argv);
    }

    if (!argv[optind]) {
        argv[optind] = (char*) RESOURCE_ROOT "/usr/share/font.psf";
    }
    if (!save_destination) {
        save_destination = argv[optind];
    }
    auto font = create_new_font ? PSF::Font::create_blank() : PSF::Font::try_create_from_path(argv[optind]);
    if (!font) {
        error_log("psfedit: `{}' is not a valid psf font file", argv[optind]);
        return 1;
    }

    auto app = App::Application::create();

    auto window = App::Window::create(nullptr, 250, 150, 500, 600, "PSF Edit");
    auto& main_widget = window->set_main_widget<App::Widget>();

    auto& layout = main_widget.set_layout_engine<App::VerticalFlexLayoutEngine>();
    auto& glyph_editor = layout.add<GlyphEditorWidget>(8, 16, font);
    glyph_editor.set_bitset(&font->bitset_for_glyph_id(0), 0);

    auto& glyph_widget = layout.add<App::Widget>();
    auto& row_layout = glyph_widget.set_layout_engine<App::VerticalFlexLayoutEngine>();
    for (int i = 0; i < 16; i++) {
        auto& row_widget = row_layout.add<App::Widget>();
        auto& col_layout = row_widget.set_layout_engine<App::HorizontalFlexLayoutEngine>();
        for (int j = 0; j < 16; j++) {
            int code_point = i * 16 + j;
            auto& button = col_layout.add<App::Button>(String(static_cast<char>(code_point)));
            button.set_font(font);
            button.on<App::ClickEvent>({}, [&, code_point](auto&) {
                glyph_editor.set_bitset(&font->bitset_for_glyph_id(code_point), code_point);
            });
        }
    }

    auto& save_button = layout.add<App::Button>("Save");
    save_button.set_layout_constraint({ App::LayoutConstraint::AutoSize, 24 });
    save_button.on<App::ClickEvent>({}, [&](auto&) {
        if (!font->save_to_file(save_destination)) {
            fprintf(stderr, "psfedit: Failed to save font to `%s'\n", save_destination);
        }
    });

    app->enter();
    return 0;
}
