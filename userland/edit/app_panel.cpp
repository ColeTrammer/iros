#include <app/box_layout.h>
#include <app/event.h>
#include <app/text_label.h>
#include <app/window.h>
#include <clipboard/connection.h>
#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>

#include "app_panel.h"
#include "document.h"
#include "key_press.h"

SearchWidget::SearchWidget() {}

SearchWidget::~SearchWidget() {}

void SearchWidget::render() {
    Widget::render();

    Renderer renderer(*window()->pixels());
    renderer.draw_rect(rect(), ColorValue::White);
}

AppPanel& SearchWidget::panel() {
    if (!m_panel) {
        auto& layout = set_layout<App::HorizontalBoxLayout>();
        auto& label = layout.add<App::TextLabel>("Find:");
        label.set_preferred_size({ 46, App::Size::Auto });

        m_panel = layout.add<AppPanel>(false).shared_from_this();
    }
    return *m_panel;
}

AppPanel::AppPanel(bool main_panel) : m_main_panel(main_panel) {}

AppPanel::~AppPanel() {}

AppPanel& AppPanel::ensure_search_panel() {
    assert(m_main_panel);
    if (!m_search_widget) {
        m_search_widget = SearchWidget::create(shared_from_this());
        m_search_widget->set_hidden(true);
    }
    return m_search_widget->panel();
}

void AppPanel::clear() {
    m_cells.resize(m_rows * m_cols);
    for (auto& cell : m_cells) {
        cell.c = ' ';
        cell.dirty = true;
        cell.metadata = CharacterMetadata();
    }
}

void AppPanel::set_text_at(int row, int col, char c, CharacterMetadata metadata) {
    auto& cell = m_cells[index(row, col)];
    if (cell.c == c && cell.metadata == metadata) {
        return;
    }

    m_cells[index(row, col)] = { c, true, metadata };
}

void AppPanel::flush() {
    invalidate();
}

int AppPanel::enter() {
    window()->set_focused_widget(*this);
    return 0;
}

void AppPanel::quit() {
    if (on_quit) {
        on_quit();
    }
}

void AppPanel::send_status_message(String) {}

Maybe<String> AppPanel::prompt(const String&) {
    return {};
}

void AppPanel::enter_search(String starting_text) {
    if (!m_main_panel) {
        return;
    }

    ensure_search_panel().set_document(Document::create_single_line(ensure_search_panel(), move(starting_text)));
    ensure_search_panel().document()->on_change = [this] {
        auto contents = ensure_search_panel().document()->content_string();
        document()->set_search_text(move(contents));
        document()->display_if_needed();
    };
    ensure_search_panel().document()->on_submit = [this] {
        document()->move_cursor_to_next_search_match();
        document()->display_if_needed();
    };
    ensure_search_panel().document()->on_escape_press = [this] {
        document()->set_search_text("");
        if (document()->on_escape_press) {
            document()->on_escape_press();
        }
        window()->set_focused_widget(*this);
    };
    ensure_search_panel().on_quit = [this] {
        document()->set_search_text("");
        if (document()->on_escape_press) {
            document()->on_escape_press();
        }
        window()->set_focused_widget(*this);
    };

    m_search_widget->set_hidden(false);
    window()->set_focused_widget(ensure_search_panel());
}

void AppPanel::notify_now_is_a_good_time_to_draw_cursor() {
    if (m_cursor_dirty) {
        flush();
    }
}

void AppPanel::notify_line_count_changed() {}

void AppPanel::set_clipboard_contents(String text, bool is_whole_line) {
    m_prev_clipboard_contents = move(text);
    m_prev_clipboard_contents_were_whole_line = is_whole_line;
    Clipboard::Connection::the().set_clipboard_contents_to_text(m_prev_clipboard_contents);
}

String AppPanel::clipboard_contents(bool& is_whole_line) const {
    auto contents = Clipboard::Connection::the().get_clipboard_contents_as_text();
    if (!contents.has_value()) {
        is_whole_line = m_prev_clipboard_contents_were_whole_line;
        return m_prev_clipboard_contents;
    }

    auto& ret = contents.value();
    if (ret == m_prev_clipboard_contents) {
        is_whole_line = m_prev_clipboard_contents_were_whole_line;
    } else {
        m_prev_clipboard_contents = "";
        is_whole_line = m_prev_clipboard_contents_were_whole_line = false;
    }
    return move(ret);
}

void AppPanel::set_cursor(int row, int col) {
    if (m_cursor_row == row && m_cursor_col == col) {
        return;
    }

    m_cursor_row = row;
    m_cursor_col = col;
    m_cursor_dirty = true;
}

void AppPanel::do_open_prompt() {}

void AppPanel::render_cursor(Renderer& renderer) {
    if (this != window()->focused_widget().get()) {
        return;
    }

    int cursor_x = rect().x() + m_cursor_col * col_width();
    int cursor_y = rect().y() + m_cursor_row * row_height();
    for (int y = cursor_y; y < cursor_y + row_height(); y++) {
        renderer.pixels().put_pixel(cursor_x, y, ColorValue::White);
    }

    m_last_drawn_cursor_col = m_cursor_col;
    m_last_drawn_cursor_row = m_cursor_row;
    m_cursor_dirty = false;
}

void AppPanel::render_cell(Renderer& renderer, int x, int y, CellData& cell) {
    RenderingInfo info = rendering_info_for_metadata(cell.metadata);

    Color fg = info.fg.has_value() ? Color(info.fg.value()) : Color(VGA_COLOR_LIGHT_GREY);
    Color bg = info.bg.has_value() ? Color(info.bg.value()) : ColorValue::Black;

    renderer.fill_rect(x, y, col_width(), row_height(), bg);
    renderer.render_text(x, y, String(cell.c), fg, info.bold ? Font::bold_font() : Font::default_font());
}

void AppPanel::render() {
    Renderer renderer(*window()->pixels());

    auto total_width = cols() * col_width();
    auto total_height = rows() * row_height();
    Rect bottom_extra_rect = { rect().x(), rect().y() + total_height, total_width, rect().height() - total_height };
    Rect right_extra_rect = { rect().x() + total_width, rect().y(), rect().width() - total_width, rect().height() };
    renderer.fill_rect(bottom_extra_rect, ColorValue::Black);
    renderer.fill_rect(right_extra_rect, ColorValue::Black);

    for (int r = 0; r < rows(); r++) {
        for (int c = 0; c < cols(); c++) {
            auto& cell = m_cells[index(r, c)];
            if (cell.dirty || (r == m_last_drawn_cursor_row && c == m_last_drawn_cursor_col)) {
                cell.dirty = false;
                render_cell(renderer, rect().x() + c * col_width(), rect().y() + r * row_height(), cell);
            }
        }
    }

    render_cursor(renderer);
    Widget::render();
}

void AppPanel::on_key_event(App::KeyEvent& event) {
    if (!document() || !event.key_down()) {
        return;
    }

    int modifiers = (event.shift_down() ? KeyPress::Modifier::Shift : 0) | (event.alt_down() ? KeyPress::Modifier::Alt : 0) |
                    (event.control_down() ? KeyPress::Modifier::Control : 0);
    int key = event.ascii();
    switch (event.key()) {
        case KEY_CURSOR_LEFT:
            key = KeyPress::Key::LeftArrow;
            break;
        case KEY_CURSOR_RIGHT:
            key = KeyPress::Key::RightArrow;
            break;
        case KEY_CURSOR_UP:
            key = KeyPress::Key::UpArrow;
            break;
        case KEY_CURSOR_DOWN:
            key = KeyPress::Key::DownArrow;
            break;
        case KEY_HOME:
            key = KeyPress::Key::Home;
            break;
        case KEY_END:
            key = KeyPress::Key::End;
            break;
        case KEY_BACKSPACE:
            key = KeyPress::Key::Backspace;
            break;
        case KEY_DELETE:
            key = KeyPress::Key::Delete;
            break;
        case KEY_ENTER:
            key = KeyPress::Key::Enter;
            break;
        case KEY_INSERT:
            key = KeyPress::Key::Insert;
            break;
        case KEY_ESC:
            key = KeyPress::Key::Escape;
            break;
        case KEY_PAGE_UP:
            key = KeyPress::Key::PageUp;
            break;
        case KEY_PAGE_DOWN:
            key = KeyPress::Key::PageDown;
            break;
        case KEY_F1:
            key = KeyPress::Key::F1;
            break;
        case KEY_F2:
            key = KeyPress::Key::F2;
            break;
        case KEY_F3:
            key = KeyPress::Key::F3;
            break;
        case KEY_F4:
            key = KeyPress::Key::F4;
            break;
        case KEY_F5:
            key = KeyPress::Key::F5;
            break;
        case KEY_F6:
            key = KeyPress::Key::F6;
            break;
        case KEY_F7:
            key = KeyPress::Key::F7;
            break;
        case KEY_F8:
            key = KeyPress::Key::F8;
            break;
        case KEY_F9:
            key = KeyPress::Key::F9;
            break;
        case KEY_F10:
            key = KeyPress::Key::F10;
            break;
        case KEY_F11:
            key = KeyPress::Key::F11;
            break;
        case KEY_F12:
            key = KeyPress::Key::F12;
            break;
        default:
            break;
    }

    if (!key) {
        return;
    }

    document()->notify_key_pressed({ modifiers, key });
}

void AppPanel::document_did_change() {
    if (document()) {
        notify_line_count_changed();

        if (m_main_panel) {
            document()->on_escape_press = [this] {
                if (m_search_widget) {
                    m_search_widget->set_hidden(true);
                    for (auto r = 0; r < rows(); r++) {
                        for (auto c = 0; c < cols(); c++) {
                            auto& cell = m_cells[r * cols() + c];
                            auto x = c * col_width();
                            auto y = r * row_height();
                            Rect cell_rect { x, y, col_width(), row_height() };
                            if (m_search_widget->rect().intersects(cell_rect)) {
                                cell.dirty = true;
                            }
                        }
                    }
                }
            };
        }
        document()->display();
    }
}

void AppPanel::on_resize() {
    m_rows = rect().height() / row_height();
    m_cols = rect().width() / col_width();
    clear();
    if (document()) {
        document()->notify_panel_size_changed();
    }

    if (m_main_panel) {
        ensure_search_panel();
        constexpr int panel_height = 28;
        m_search_widget->set_rect({ rect().x(), rect().y() + rect().height() - panel_height, rect().width(), panel_height });
    }
}

void AppPanel::on_focused() {
    invalidate();
}
