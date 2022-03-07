
#pragma once

#include <app/base/widget.h>
#include <edit/absolute_position.h>
#include <edit/character_metadata.h>
#include <edit/display_bridge.h>
#include <edit/display_interface.h>
#include <edit/forward.h>
#include <edit/multicursor.h>
#include <edit/rendered_line.h>
#include <edit/suggestions.h>
#include <edit/text_range_collection.h>
#include <eventloop/forward.h>
#include <graphics/color.h>
#include <graphics/forward.h>
#include <liim/forward.h>
#include <liim/function.h>
#include <liim/maybe.h>
#include <liim/pointers.h>

APP_EVENT(Edit, SplitDisplayEvent, App::Event, (), (), ())
APP_EVENT(Edit, NewDisplayEvent, App::Event, (), (), ())

namespace Edit {
enum class AutoCompleteMode { Never, Always };

class Display : public App::Base::Widget {
    APP_OBJECT(Display)

    APP_EMITS(App::Base::Widget, SplitDisplayEvent, NewDisplayEvent)

    EDIT_DISPLAY_BRIDGE_INTERFACE_FORWARD(bridge())

public:
    virtual void initialize() override;
    virtual ~Display();

    struct RenderingInfo {
        Maybe<Color> fg;
        Maybe<Color> bg;
        bool bold { false };
        bool main_cursor { false };
        bool secondary_cursor { false };
    };

    // os_2 reflect begin
    Edit::AbsolutePosition scroll_offset() const { return m_scroll_offset; }
    void set_scroll_offset(const Edit::AbsolutePosition& offset);

    void scroll_up(int times) { scroll(-times, 0); }
    void scroll_down(int times) { scroll(times, 0); }
    void scroll_left(int times) { scroll(0, -times); }
    void scroll_right(int times) { scroll(0, times); }
    void scroll(int vertical, int horizontal);

    void scroll_cursor_into_view(Edit::Cursor& cursor);
    void center_on_cursor(Edit::Cursor& cursor);

    App::ObjectBoundCoroutine save();
    App::ObjectBoundCoroutine go_to_line();

    void invalidate_metadata() { invalidate_all_line_rects(); }

    void set_document(SharedPtr<Edit::Document> document);

    Edit::Document* document() { return m_document.get(); }
    const Edit::Document* document() const { return m_document.get(); }

    SharedPtr<Edit::Document> document_as_shared() const { return m_document; }

    Edit::Cursor& main_cursor() { return m_cursors.main_cursor(); }
    const Edit::Cursor& main_cursor() const { return m_cursors.main_cursor(); }

    Edit::MultiCursor& cursors() { return m_cursors; }
    const Edit::MultiCursor& cursors() const { return m_cursors; }

    Edit::AutoCompleteMode auto_complete_mode() const { return m_auto_complete_mode; }
    void set_auto_complete_mode(Edit::AutoCompleteMode mode) { m_auto_complete_mode = mode; }

    bool preview_auto_complete() const { return m_preview_auto_complete; }
    void set_preview_auto_complete(bool b);

    bool show_line_numbers() const { return m_show_line_numbers; }
    void toggle_show_line_numbers();
    void set_show_line_numbers(bool b);

    bool word_wrap_enabled() const { return m_word_wrap_enabled; }
    void toggle_word_wrap_enabled();
    void set_word_wrap_enabled(bool b);

    Edit::Suggestions& suggestions() { return m_suggestions; }
    const Edit::Suggestions& suggestions() const { return m_suggestions; }

    void compute_suggestions();
    void set_suggestions(Vector<Edit::Suggestion> suggestions);

    Edit::Display::RenderingInfo rendering_info_for_metadata(const Edit::CharacterMetadata& metadata) const;

    void render_lines();

    Edit::RenderedLine& rendered_line_at_index(int index);
    const Edit::RenderedLine& rendered_line_at_index(int index) const { return const_cast<Display&>(*this).rendered_line_at_index(index); }

    int absolute_col_offset_of_index(const Edit::TextIndex& index) const;
    int rendered_line_count(int line_index) const;

    Edit::TextIndex prev_index_into_line(const Edit::TextIndex& index) const;
    Edit::TextIndex next_index_into_line(const Edit::TextIndex& index) const;

    Edit::TextIndex text_index_at_absolute_position(const Edit::AbsolutePosition& position) const;
    Edit::TextIndex text_index_at_display_position(const Edit::DisplayPosition& position) const;

    Edit::AbsolutePosition display_to_absolute_position(const Edit::DisplayPosition& display_position) const;
    Edit::AbsolutePosition absolute_position_of_index(const Edit::TextIndex& index) const;

    Edit::DisplayPosition absolute_to_display_position(const Edit::AbsolutePosition& absolute_position) const;
    Edit::DisplayPosition display_position_of_index(const Edit::TextIndex& index) const;

    void invalidate_all_lines();
    void invalidate_line(int line_index);

    void start_input(bool should_save_cursor_state);
    void finish_input(bool should_scroll_cursor_into_view);

    void clear_search();
    void update_search_results();
    void move_cursor_to_next_search_match();
    void select_next_word_at_cursor();
    void replace_next_search_match(const String& replacement);

    void set_search_text(String text);
    const Edit::TextRangeCollection& search_results() const { return m_search_results; }
    const String& search_text() const { return m_search_text; }
    const String& previous_search_text() const { return m_previous_search_text; }
    // os_2 reflect end

    DisplayBridge& bridge() { return *m_bridge; }
    const DisplayBridge& bridge() const { return *m_bridge; }

protected:
    Display(SharedPtr<App::Base::WidgetBridge> widget_bridge, SharedPtr<DisplayBridge> display_bridge);

    void install_document_listeners(Document& document);
    void uninstall_document_listeners(Document& document);

private:
    void clamp_scroll_offset();

    SharedPtr<Document> m_document;
    Vector<RenderedLine> m_rendered_lines;

    SharedPtr<DisplayBridge> m_bridge;

    MultiCursor m_cursors;

    Suggestions m_suggestions;
    AutoCompleteMode m_auto_complete_mode { AutoCompleteMode::Never };
    bool m_preview_auto_complete { false };

    String m_previous_search_text;
    String m_search_text;
    TextRangeCollection m_search_results;
    int m_search_result_index { 0 };

    bool m_word_wrap_enabled { false };
    bool m_show_line_numbers { false };

    AbsolutePosition m_scroll_offset;
};
}
