#include <edit/display.h>
#include <edit/document.h>
#include <edit/document_type.h>
#include <edit/rendered_line.h>
#include <errno.h>
#include <eventloop/widget_events.h>
#include <graphics/point.h>
#include <liim/string.h>
#include <liim/vector.h>
#include <unistd.h>

namespace Edit {
Display::Display() : m_cursors { *this } {}

void Display::initialize() {
    this_widget().on_unchecked<App::ResizeEvent>({}, [this](auto&) {
        if (word_wrap_enabled()) {
            invalidate_all_lines();
            clamp_scroll_offset();
        }
    });

    this_widget().on_unchecked<App::MouseDownEvent>({}, [this](const App::MouseDownEvent& event) {
        if (!document()) {
            return false;
        }
        auto& document = *this->document();

        auto index = text_index_at_mouse_position({ event.x(), event.y() });
        if (event.left_button()) {
            start_input(true);
            cursors().remove_secondary_cursors();
            auto& cursor = main_cursor();
            document.move_cursor_to(*this, cursor, index, MovementMode::Move);
            switch (event.cyclic_count(3)) {
                case 2:
                    document.select_word_at_cursor(*this, cursor);
                    break;
                case 3:
                    document.select_line_at_cursor(*this, cursor);
                    break;
                default:
                    break;
            }
            finish_input(true);
            return true;
        }
        return false;
    });

    this_widget().on_unchecked<App::MouseMoveEvent>({}, [this](const App::MouseMoveEvent& event) {
        if (!document()) {
            return false;
        }
        auto& document = *this->document();

        auto index = text_index_at_mouse_position({ event.x(), event.y() });
        if (event.buttons_down() & App::MouseButton::Left) {
            start_input(true);
            cursors().remove_secondary_cursors();
            auto& cursor = main_cursor();
            document.move_cursor_to(*this, cursor, index, MovementMode::Select);
            finish_input(true);
            return true;
        }
        return false;
    });

    this_widget().on_unchecked<App::MouseScrollEvent>({}, [this](const App::MouseScrollEvent& event) {
        if (!document()) {
            return false;
        }
        start_input(true);
        scroll(2 * event.z(), 0);
        finish_input(false);
        return true;
    });

    this_widget().on_unchecked<App::TextEvent>({}, [this](const App::TextEvent& event) {
        if (!document()) {
            return false;
        }
        auto& document = *this->document();

        start_input(true);
        document.insert_text_at_cursor(*this, event.text());
        finish_input(true);
        return true;
    });
}

Display::~Display() {}

void Display::set_document(SharedPtr<Document> document) {
    if (m_document == document) {
        return;
    }

    if (m_document) {
        uninstall_document_listeners(*m_document);
        m_search_results.clear();
    }
    m_document = move(document);
    if (m_document) {
        install_document_listeners(*m_document);
        clear_search();
    }

    m_cursors.remove_secondary_cursors();
    m_cursors.main_cursor().reset();
    m_rendered_lines.resize(m_document->line_count());
    invalidate_all_lines();
    set_scroll_offset({});
    hide_suggestions_panel();
    document_did_change();
}

void Display::set_suggestions(Vector<Suggestion> suggestions) {
    auto old_suggestion_range = m_suggestions.current_text_range();
    m_suggestions.set_suggestions(move(suggestions));

    auto cursor_index = main_cursor().index();
    auto index_for_suggestion = TextIndex { cursor_index.line_index(), max(cursor_index.index_into_line() - 1, 0) };
    m_suggestions.set_current_text_range(document()->syntax_highlighting_info().range_at_text_index(index_for_suggestion));

    m_suggestions.compute_matches(*document(), main_cursor());

    suggestions_did_change(old_suggestion_range);
}

void Display::compute_suggestions() {
    do_compute_suggestions();
}

void Display::clamp_scroll_offset() {
    if (!document()) {
        set_scroll_offset({});
        return;
    }

    if (scroll_offset().line_index() >= document()->line_count()) {
        set_scroll_offset(
            { document()->last_line_index(), rendered_line_count(document()->last_line_index()) - 1, scroll_offset().relative_col() });
        return;
    }

    auto rendered_line_count = this->rendered_line_count(scroll_offset().line_index());
    if (scroll_offset().relative_row() >= rendered_line_count) {
        set_scroll_offset({ scroll_offset().line_index(), rendered_line_count - 1, 0 });
    }
}

void Display::set_scroll_offset(const AbsolutePosition& offset) {
    if (m_scroll_offset == offset) {
        return;
    }

    m_scroll_offset = offset;
    invalidate_all_line_rects();
}

void Display::scroll(int vertical, int horizontal) {
    set_scroll_offset(display_to_absolute_position({ vertical, horizontal }));
}

void Display::center_on_cursor(Cursor& cursor) {
    auto position = cursor.absolute_position(*this);
    position.set_relative_col(0);

    set_scroll_offset(position);
    scroll_up(rows() / 2);
}

void Display::scroll_cursor_into_view(Cursor& cursor) {
    // NOTE: this is a fast path to prevent calling display_position_of_index() when the position is very far
    //       from the display's scroll offset. If we know for sure the cursor is not on the display, we can
    //       first move the cursor to the display and then adjust if needed.
    if (cursor.index().line_index() < scroll_offset().line_index()) {
        auto relative_row = cursor.relative_position(*this).row();
        set_scroll_offset({ cursor.line_index(), relative_row, scroll_offset().relative_col() });
    } else if (cursor.index().line_index() > scroll_offset().line_index() + rows()) {
        auto relative_row = cursor.relative_position(*this).row();
        set_scroll_offset({ cursor.line_index(), relative_row, scroll_offset().relative_col() });
        scroll_up(rows() - 1);
    }

    auto cursor_position = display_position_of_index(cursor.index());
    if (cursor_position.row() < 0) {
        scroll_up(-cursor_position.row());
    } else if (cursor_position.row() >= rows()) {
        scroll_down(cursor_position.row() - rows() + 1);
    }

    if (cursor_position.col() < 0) {
        scroll_left(-cursor_position.col());
    } else if (cursor_position.col() >= cols()) {
        scroll_right(cursor_position.col() - cols() + 1);
    }
}

RenderedLine& Display::rendered_line_at_index(int index) {
    assert(index < m_rendered_lines.size());

    auto& line = m_rendered_lines[index];
    if (!line.is_up_to_date()) {
        m_rendered_lines[index] = compose_line(document()->line_at_index(index));
    }
    return line;
}

void Display::invalidate_all_lines() {
    m_rendered_lines.clear();
    if (auto doc = document()) {
        m_rendered_lines.resize(doc->line_count());
    }
    invalidate_all_line_rects();
}

void Display::invalidate_line(int line_index) {
    m_rendered_lines[line_index].invalidate();
    invalidate_all_line_rects();
}

void Display::toggle_show_line_numbers() {
    set_show_line_numbers(!m_show_line_numbers);
}

void Display::set_preview_auto_complete(bool b) {
    if (m_preview_auto_complete == b) {
        return;
    }

    m_preview_auto_complete = b;
    if (document()) {
        invalidate_line(main_cursor().line_index());
    }
}

void Display::set_show_line_numbers(bool b) {
    if (m_show_line_numbers == b) {
        return;
    }

    m_show_line_numbers = b;
    did_set_show_line_numbers();
}

void Display::set_word_wrap_enabled(bool b) {
    if (m_word_wrap_enabled == b) {
        return;
    }

    m_word_wrap_enabled = b;
    if (document()) {
        invalidate_all_lines();
        for (auto& cursor : cursors()) {
            cursor.compute_max_col(*this);
        }
        set_scroll_offset({ scroll_offset().line_index(), 0, 0 });
        scroll_cursor_into_view(main_cursor());
    }
}

void Display::toggle_word_wrap_enabled() {
    set_word_wrap_enabled(!m_word_wrap_enabled);
}

void Display::replace_next_search_match(const String& replacement) {
    start_input(true);
    document()->replace_next_search_match(*this, replacement);
    finish_input(true);
}

void Display::move_cursor_to_next_search_match() {
    if (!document() || m_search_results.empty()) {
        return;
    }

    start_input(true);

    cursors().remove_secondary_cursors();
    auto& cursor = main_cursor();

    if (m_search_result_index >= m_search_results.size()) {
        m_search_result_index = 0;
        document()->move_cursor_to_document_start(*this, cursor);
    }

    while (m_search_results.range(m_search_result_index).ends_before(cursor.index())) {
        m_search_result_index++;
        if (m_search_result_index == m_search_results.size()) {
            m_search_result_index = 0;
            document()->move_cursor_to_document_start(*this, cursor);
        }
    }

    document()->move_cursor_to(*this, cursor, m_search_results.range(m_search_result_index).start());
    document()->move_cursor_to(*this, cursor, m_search_results.range(m_search_result_index).end(), MovementMode::Select);
    m_search_result_index++;
    finish_input(true);
}

void Display::select_next_word_at_cursor() {
    if (!document()) {
        return;
    }

    auto& main_cursor = this->main_cursor();
    if (main_cursor.selection().empty()) {
        document()->select_word_at_cursor(*this, main_cursor);
        if (main_cursor.selection().empty()) {
            return;
        }

        auto search_text = document()->selection_text(main_cursor);
        set_search_text(move(search_text));

        // Set m_search_result_index to point just past the current cursor.
        while (m_search_result_index < m_search_results.size() &&
               m_search_results.range(m_search_result_index).ends_before(main_cursor.index())) {
            m_search_result_index++;
        }
        m_search_result_index %= m_search_results.size();
        return;
    } else {
        set_search_text(document()->selection_text(main_cursor));
    }

    auto& result = m_search_results.range(m_search_result_index);
    auto* added_cursor = cursors().add_cursor_at(*document(), result.end(), result.start());
    if (added_cursor) {
        scroll_cursor_into_view(*added_cursor);
    }

    ++m_search_result_index;
    m_search_result_index %= m_search_results.size();
}

void Display::update_search_results() {
    if (!document()) {
        return;
    }

    invalidate_metadata();

    m_search_results.clear();
    if (m_search_text.empty()) {
        return;
    }

    for (int i = 0; i < document()->line_count(); i++) {
        document()->line_at_index(i).search(*document(), i, m_search_text, m_search_results);
    }

    m_search_result_index = 0;
    while (m_search_result_index < m_search_results.size() &&
           main_cursor().index() < m_search_results.range(m_search_result_index).start()) {
        m_search_result_index++;
    }
}

void Display::clear_search() {
    if (!document()) {
        return;
    }

    invalidate_metadata();
    m_search_result_index = 0;
    m_search_results.clear();

    m_previous_search_text = move(m_search_text);
}

void Display::set_search_text(String text) {
    if (text == m_search_text) {
        return;
    }

    m_search_text = move(text);
    update_search_results();
}

void Display::install_document_listeners(Document& new_document) {
    new_document.on<DeleteLines>(this_widget(), [this](const DeleteLines& event) {
        m_rendered_lines.remove_count(event.line_index(), event.line_count());
        invalidate_all_line_rects();
        clamp_scroll_offset();
    });

    new_document.on<AddLines>(this_widget(), [this](const AddLines& event) {
        auto new_rendered_lines = Vector<RenderedLine> {};
        new_rendered_lines.resize(event.line_count());

        m_rendered_lines.insert(move(new_rendered_lines), event.line_index());
        invalidate_all_line_rects();
    });

    new_document.on<SplitLines>(this_widget(), [this](const SplitLines& event) {
        m_rendered_lines.insert(RenderedLine {}, event.line_index() + 1);
        invalidate_line(event.line_index());
        invalidate_all_line_rects();
    });

    new_document.on<MergeLines>(this_widget(), [this](const MergeLines& event) {
        m_rendered_lines.remove(event.second_line_index());
        invalidate_line(event.first_line_index());
        invalidate_all_line_rects();
        clamp_scroll_offset();
    });

    new_document.on<AddToLine>(this_widget(), [this](const AddToLine& event) {
        invalidate_line(event.line_index());
    });

    new_document.on<DeleteFromLine>(this_widget(), [this](const DeleteFromLine& event) {
        invalidate_line(event.line_index());
        clamp_scroll_offset();
    });

    new_document.on<MoveLineTo>(this_widget(), [this](const MoveLineTo& event) {
        auto line_min = min(event.line(), event.destination());
        auto line_max = max(event.line(), event.destination());

        if (event.line() > event.destination()) {
            m_rendered_lines.rotate_right(line_min, line_max + 1);
        } else {
            m_rendered_lines.rotate_left(line_min, line_max + 1);
        }

        invalidate_all_line_rects();
        clamp_scroll_offset();
    });

    new_document.on<SyntaxHighlightingChanged>(this_widget(), [this](auto&) {
        invalidate_metadata();
    });

    new_document.on<Change>(this_widget(), [this](auto&) {
        update_search_results();
    });

    cursors().install_document_listeners(new_document);
}

void Display::uninstall_document_listeners(Document& document) {
    document.remove_listener(this_widget());
}

void Display::render_lines() {
    if (!document()) {
        return;
    }

    auto render_position = scroll_offset();
    for (int row_in_display = 0; row_in_display < rows() && render_position.line_index() < document()->line_count();) {
        auto& line = rendered_line_at_index(render_position.line_index());
        row_in_display += line.render(*this, { render_position.line_index(), 0 }, render_position.relative_col(),
                                      render_position.relative_row(), row_in_display);
        render_position.set_line_index(render_position.line_index() + 1);
        render_position.set_relative_row(0);
    }
}

int Display::absolute_col_offset_of_index(const TextIndex& index) const {
    return rendered_line_at_index(index.line_index()).absolute_col_offset_of_index(index);
}

int Display::rendered_line_count(int line_index) const {
    return rendered_line_at_index(line_index).rendered_line_count();
}

TextIndex Display::prev_index_into_line(const TextIndex& index) const {
    return rendered_line_at_index(index.line_index()).prev_index_into_line(index);
}

TextIndex Display::next_index_into_line(const TextIndex& index) const {
    return rendered_line_at_index(index.line_index()).next_index_into_line(index);
}

TextIndex Display::text_index_at_absolute_position(const AbsolutePosition& position) const {
    return rendered_line_at_index(position.line_index()).index_of_absolute_position(position);
}

TextIndex Display::text_index_at_display_position(const DisplayPosition& position) const {
    return text_index_at_absolute_position(display_to_absolute_position(position));
}

AbsolutePosition Display::display_to_absolute_position(const DisplayPosition& display_position) const {
    auto absolute_position = scroll_offset();

    auto position = display_position;
    while (position.row() < 0) {
        if (absolute_position.line_index() == 0 && absolute_position.relative_row() == 0) {
            break;
        }
        if (absolute_position.relative_row() == 0) {
            absolute_position.set_line_index(absolute_position.line_index() - 1);
            absolute_position.set_relative_row(rendered_line_count(absolute_position.line_index()) - 1);
        } else {
            absolute_position.set_relative_row(absolute_position.relative_row() - 1);
        }
        position.set_row(position.row() + 1);
    }
    while (position.row() > 0) {
        if (absolute_position.line_index() == document()->last_line_index() &&
            absolute_position.relative_row() == rendered_line_count(document()->last_line_index()) - 1) {
            break;
        }
        if (absolute_position.relative_row() == rendered_line_count(absolute_position.line_index()) - 1) {
            absolute_position.set_line_index(absolute_position.line_index() + 1);
            absolute_position.set_relative_row(0);
        } else {
            absolute_position.set_relative_row(absolute_position.relative_row() + 1);
        }
        position.set_row(position.row() - 1);
    }

    absolute_position.set_relative_col(absolute_position.relative_col() + position.col());
    return absolute_position;
}

AbsolutePosition Display::absolute_position_of_index(const TextIndex& index) const {
    auto position = rendered_line_at_index(index.line_index()).relative_position_of_index(index);
    return { index.line_index(), position.row(), position.col() };
}

DisplayPosition Display::absolute_to_display_position(const AbsolutePosition& position) const {
    if (!word_wrap_enabled()) {
        return { position.line_index() - scroll_offset().line_index(), position.relative_col() - scroll_offset().relative_col() };
    }

    int row_offset = 0;
    if (position.line_index() <= scroll_offset().line_index()) {
        for (int line_index = scroll_offset().line_index() - 1; line_index >= position.line_index(); line_index--) {
            row_offset -= rendered_line_count(line_index);
        }
    } else {
        for (int line_index = scroll_offset().line_index(); line_index < position.line_index(); line_index++) {
            row_offset += rendered_line_count(line_index);
        }
    }
    return { row_offset + position.relative_row() - scroll_offset().relative_row(),
             position.relative_col() - scroll_offset().relative_col() };
}

DisplayPosition Display::display_position_of_index(const TextIndex& index) const {
    return absolute_to_display_position(absolute_position_of_index(index));
}

void Display::start_input(bool should_save_cursor_state) {
    if (should_save_cursor_state) {
        cursors().cursor_save();
    }
}

void Display::finish_input(bool should_scroll_cursor_into_view) {
    cursors().remove_duplicate_cursors();

    if (should_scroll_cursor_into_view) {
        scroll_cursor_into_view(main_cursor());
    }

    compute_suggestions();
    if (preview_auto_complete()) {
        invalidate_line(main_cursor().line_index());
    }

    cursors().invalidate_based_on_last_snapshot(*document());
}

Display::RenderingInfo Display::rendering_info_for_metadata(const CharacterMetadata& metadata) const {
    RenderingInfo info;
    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxComment) {
        info.fg = { VGA_COLOR_DARK_GREY };
    }

    if (metadata.highlighted()) {
        info.fg = { VGA_COLOR_BLACK };
        info.bg = { VGA_COLOR_YELLOW };
    }

    if (metadata.selected()) {
        info.fg = {};
        info.bg = { VGA_COLOR_DARK_GREY };
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxOperator) {
        info.fg = { VGA_COLOR_CYAN };
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxKeyword) {
        info.fg = { VGA_COLOR_MAGENTA };
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxNumber) {
        info.fg = { VGA_COLOR_RED };
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxIdentifier) {
        info.fg = { VGA_COLOR_YELLOW };
        info.bold = true;
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxString) {
        info.fg = { VGA_COLOR_GREEN };
    }

    if (metadata.syntax_highlighting() & CharacterMetadata::Flags::SyntaxImportant) {
        info.bold = true;
    }

    if (metadata.highlighted() && metadata.selected()) {
        info.fg = { VGA_COLOR_YELLOW };
        info.bg = { VGA_COLOR_DARK_GREY };
    }

    if (metadata.auto_complete_preview()) {
        info.fg = { VGA_COLOR_DARK_GREY };
    }

    if (metadata.main_cursor()) {
        info.main_cursor = true;
    }

    if (metadata.secondary_cursor()) {
        info.secondary_cursor = true;
    }

    return info;
}

App::ObjectBoundCoroutine Display::go_to_line() {
    auto maybe_result = co_await prompt("Go to line: ");
    if (!maybe_result.has_value()) {
        co_return;
    }

    auto& result = maybe_result.value();
    char* end_ptr = result.string();
    long line_number = strtol(result.string(), &end_ptr, 10);
    if (errno == ERANGE || end_ptr != result.string() + result.size() || line_number < 1 || line_number > document()->line_count()) {
        send_status_message(format("Line `{}' is not between 1 and {}", result, document()->line_count()));
        co_return;
    }

    auto& cursor = main_cursor();
    cursor.clear_selection();
    cursor.set_line_index(line_number - 1);

    center_on_cursor(cursor);

    document()->move_cursor_to_line_start(*this, cursor);
}

App::ObjectBoundCoroutine Display::save() {
    if (document()->name().empty()) {
        auto result = co_await prompt("Save as: ");
        if (!result.has_value()) {
            co_return;
        }

        if (access(result.value().string(), F_OK) == 0) {
            auto ok = co_await prompt(format("Are you sure you want to overwrite file `{}'? ", *result));
            if (!ok.has_value() || (ok.value() != "y" && ok.value() != "yes")) {
                co_return;
            }
        }

        document()->set_name(move(result.value()));
    }

    assert(!document()->name().empty());

    if (access(document()->name().string(), W_OK)) {
        if (errno != ENOENT) {
            send_status_message(format("Permission to write file `{}' denied", document()->name()));
            co_return;
        }
    }

    FILE* file = fopen(document()->name().string(), "w");
    if (!file) {
        send_status_message(format("Failed to save - `{}'", strerror(errno)));
        co_return;
    }

    if (document()->line_count() != 1 || !document()->first_line().empty()) {
        for (int i = 0; i < document()->line_count(); i++) {
            fprintf(file, "%s\n", document()->line_at_index(i).contents().string());
        }
    }

    if (ferror(file)) {
        send_status_message(format("Failed to write to disk - `{}'", strerror(errno)));
        fclose(file);
        co_return;
    }

    if (fclose(file)) {
        send_status_message(format("Failed to sync to disk - `{}'", strerror(errno)));
        co_return;
    }

    send_status_message(format("Successfully saved file: `{}'", document()->name()));
    document()->set_was_modified(false);
}

Task<Maybe<String>> Display::prompt(String, String) {
    co_return {};
}

App::ObjectBoundCoroutine Display::do_open_prompt() {
    co_return;
}
}
