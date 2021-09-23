#include <edit/actions.h>
#include <edit/display.h>
#include <edit/document.h>
#include <edit/keyboard_action.h>

namespace Edit {
void init_actions() {
    register_document_keyboard_action("Swap Lines Down", { App::Key::DownArrow, App::KeyModifier::Alt }, [](Display& display) {
        auto& document = *display.document();
        document.swap_lines_at_cursor(display, SwapDirection::Down);
    });
    register_document_keyboard_action("Swap Lines Up", { App::Key::UpArrow, App::KeyModifier::Alt }, [](Display& display) {
        auto& document = *display.document();
        document.swap_lines_at_cursor(display, SwapDirection::Up);
    });

    register_document_keyboard_action("Move Cursor Left", { App::Key::LeftArrow, 0 }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_left(display, cursor);
        }
    });
    register_document_keyboard_action("Select Left", { App::Key::LeftArrow, App::KeyModifier::Shift }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_left(display, cursor, MovementMode::Select);
        }
    });
    register_document_keyboard_action("Move Cursor Right", { App::Key::RightArrow, 0 }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_right(display, cursor);
        }
    });
    register_document_keyboard_action("Select Right", { App::Key::RightArrow, App::KeyModifier::Shift }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_right(display, cursor, MovementMode::Select);
        }
    });
    register_document_keyboard_action("Move Cursor Down", { App::Key::DownArrow, 0 }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_down(display, cursor);
        }
    });
    register_document_keyboard_action("Select Down", { App::Key::DownArrow, App::KeyModifier::Shift }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_down(display, cursor, MovementMode::Select);
        }
    });
    register_document_keyboard_action("Move Cursor Up", { App::Key::UpArrow, 0 }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_up(display, cursor);
        }
    });
    register_document_keyboard_action("Select Up", { App::Key::UpArrow, App::KeyModifier::Shift }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_up(display, cursor, MovementMode::Select);
        }
    });
    register_document_keyboard_action("Move Cursor To Line Start", { App::Key::Home, 0 }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_to_line_start(display, cursor);
        }
    });
    register_document_keyboard_action("Select Line Start", { App::Key::Home, App::KeyModifier::Shift }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_to_line_start(display, cursor, MovementMode::Select);
        }
    });
    register_document_keyboard_action("Move Cursor To Line End", { App::Key::End, 0 }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_to_line_end(display, cursor);
        }
    });
    register_document_keyboard_action("Select Line End", { App::Key::End, App::KeyModifier::Shift }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_to_line_end(display, cursor, MovementMode::Select);
        }
    });
    register_document_keyboard_action("Move Cursor Page Up", { App::Key::PageUp, 0 }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_page_up(display, cursor);
        }
    });
    register_document_keyboard_action("Select Page Up", { App::Key::PageUp, App::KeyModifier::Shift }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_page_up(display, cursor, MovementMode::Select);
        }
    });
    register_document_keyboard_action("Move Cursor Page Down", { App::Key::PageDown, 0 }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_page_down(display, cursor);
        }
    });
    register_document_keyboard_action("Select Page Down", { App::Key::PageDown, App::KeyModifier::Shift }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_page_down(display, cursor, MovementMode::Select);
        }
    });

    register_document_keyboard_action("Move Cursor Left by Word", { App::Key::LeftArrow, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_left_by_word(display, cursor);
        }
    });
    register_document_keyboard_action("Select Left by Word", { App::Key::LeftArrow, App::KeyModifier::Control | App::KeyModifier::Shift },
                                      [](Display& display) {
                                          auto& document = *display.document();
                                          for (auto& cursor : display.cursors()) {
                                              document.move_cursor_left_by_word(display, cursor, MovementMode::Select);
                                          }
                                      });
    register_document_keyboard_action("Move Cursor Right by Word", { App::Key::RightArrow, App::KeyModifier::Control },
                                      [](Display& display) {
                                          auto& document = *display.document();
                                          for (auto& cursor : display.cursors()) {
                                              document.move_cursor_right_by_word(display, cursor);
                                          }
                                      });
    register_document_keyboard_action("Select Right by Word", { App::Key::RightArrow, App::KeyModifier::Control | App::KeyModifier::Shift },
                                      [](Display& display) {
                                          auto& document = *display.document();
                                          for (auto& cursor : display.cursors()) {
                                              document.move_cursor_right_by_word(display, cursor, MovementMode::Select);
                                          }
                                      });
    register_document_keyboard_action("Move Cursor to Document Start", { App::Key::Home, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_to_document_start(display, cursor);
        }
    });
    register_document_keyboard_action("Select Document Start", { App::Key::Home, App::KeyModifier::Control | App::KeyModifier::Shift },
                                      [](Display& display) {
                                          auto& document = *display.document();
                                          for (auto& cursor : display.cursors()) {
                                              document.move_cursor_to_document_start(display, cursor, MovementMode::Select);
                                          }
                                      });
    register_document_keyboard_action("Move Cursor to Document End", { App::Key::End, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.move_cursor_to_document_end(display, cursor);
        }
    });
    register_document_keyboard_action("Select Document End", { App::Key::End, App::KeyModifier::Control | App::KeyModifier::Shift },
                                      [](Display& display) {
                                          auto& document = *display.document();
                                          for (auto& cursor : display.cursors()) {
                                              document.move_cursor_to_document_end(display, cursor, MovementMode::Select);
                                          }
                                      });

    register_display_keyboard_action("Add Cursor Down", { App::Key::DownArrow, App::KeyModifier::Control | App::KeyModifier::Shift },
                                     [](Display& display) {
                                         auto& document = *display.document();
                                         if (auto* cursor = display.cursors().add_cursor(document, AddCursorMode::Down)) {
                                             document.scroll_cursor_into_view(display, *cursor);
                                         }
                                     });
    register_display_keyboard_action("Add Cursor Up", { App::Key::UpArrow, App::KeyModifier::Control | App::KeyModifier::Shift },
                                     [](Display& display) {
                                         auto& document = *display.document();
                                         if (auto* cursor = display.cursors().add_cursor(document, AddCursorMode::Up)) {
                                             document.scroll_cursor_into_view(display, *cursor);
                                         }
                                     });

    register_document_keyboard_action("Delete Left", { App::Key::Backspace, 0 }, [](Display& display) {
        auto& document = *display.document();
        document.delete_char(display, DeleteCharMode::Backspace);
    });
    register_document_keyboard_action("Delete Right", { App::Key::Delete, 0 }, [](Display& display) {
        auto& document = *display.document();
        document.delete_char(display, DeleteCharMode::Delete);
    });

    register_document_keyboard_action("Delete Word", { App::Key::D, App::KeyModifier::Alt }, [](Display& display) {
        auto& document = *display.document();
        document.delete_word(display, DeleteCharMode::Delete);
    });
    register_document_keyboard_action("Delete Word Left", { App::Key::Backspace, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        document.delete_word(display, DeleteCharMode::Backspace);
    });
    register_document_keyboard_action("Delete Word Right", { App::Key::Delete, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        document.delete_word(display, DeleteCharMode::Delete);
    });

    register_document_keyboard_action("Split Line", { App::Key::Enter, 0 }, [](Display& display) {
        auto& document = *display.document();
        if (!document.submittable() || &display.main_cursor().referenced_line(document) != &document.last_line()) {
            document.split_line_at_cursor(display);
        } else if (document.submittable()) {
            document.emit<Submit>();
        }
    });

    register_document_keyboard_action("Reset Cursors", { App::Key::Escape, 0 }, [](Display& display) {
        display.clear_search();
        display.cursors().remove_secondary_cursors();
        display.main_cursor().clear_selection();
    });

    register_document_keyboard_action("Show Suggestions", { App::Key::Space, App::KeyModifier::Control }, [](Display& display) {
        display.compute_suggestions();
        display.show_suggestions_panel();
    });
    register_document_keyboard_action("Complete Suggestion", { App::Key::Tab, 0 }, [](Display& display) {
        auto& document = *display.document();
        if (display.auto_complete_mode() == AutoCompleteMode::Always) {
            display.compute_suggestions();
            auto suggestions = display.suggestions();
            if (suggestions.size() == 1) {
                document.insert_suggestion(display, suggestions.first());
            } else if (suggestions.size() > 1) {
                display.show_suggestions_panel();
            }
            return;
        }
        document.insert_char(display, '\t');
    });

    register_display_keyboard_action("Select All", { App::Key::A, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        document.select_all(display, display.main_cursor());
    });

    register_display_keyboard_action("Select All Words at Cursor",
                                     { App::Key::D, App::KeyModifier::Control, App::KeyShortcut::IsMulti::Yes }, [](Display& display) {
                                         auto& document = *display.document();
                                         display.select_next_word_at_cursor();
                                         if (!display.search_text().empty()) {
                                             document.select_all_matches(display, display.search_results());
                                         }
                                     });
    register_display_keyboard_action("Select Next Word at Cursor", { App::Key::D, App::KeyModifier::Control }, [](Display& display) {
        display.select_next_word_at_cursor();
    });
    register_document_keyboard_action("Select Line", { App::Key::L, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        for (auto& cursor : display.cursors()) {
            document.select_line_at_cursor(display, cursor);
        }
    });

    register_document_keyboard_action("Cursor Undo", { App::Key::U, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        display.cursors().cursor_undo(document);
    });

    register_document_keyboard_action("Copy", { App::Key::C, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        document.copy(display, display.cursors());
    });
    register_document_keyboard_action("Paste", { App::Key::V, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        document.paste(display, display.cursors());
    });
    register_document_keyboard_action("Cut", { App::Key::X, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        document.cut(display, display.cursors());
    });

    register_document_keyboard_action("Redo", { App::Key::Y, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        document.redo(display);
    });
    register_document_keyboard_action("Undo", { App::Key::Z, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        document.undo(display);
    });

    register_display_keyboard_action("Scroll Down", { App::Key::DownArrow, App::KeyModifier::Control }, [](Display& display) {
        display.scroll_down(1);
    });
    register_display_keyboard_action("Scroll Up", { App::Key::UpArrow, App::KeyModifier::Control }, [](Display& display) {
        display.scroll_up(1);
    });

    register_display_keyboard_action("New Display", { App::Key::N, App::KeyModifier::Control }, [](Display& display) {
        display.this_widget().emit<Edit::NewDisplayEvent>();
    });
    register_display_keyboard_action("Split Display", { App::Key::Backslash, App::KeyModifier::Control }, [](Display& display) {
        display.this_widget().emit<Edit::SplitDisplayEvent>();
    });

    register_display_keyboard_action("Search", { App::Key::F, App::KeyModifier::Control }, [](Display& display) {
        display.enter_search(display.previous_search_text());
        display.set_search_text(display.previous_search_text());
    });

    register_display_keyboard_action("Go To Line", { App::Key::G, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        display.this_widget().start_coroutine(document.go_to_line(display));
    });

    register_display_keyboard_action("Toogle Show Line Numbers", { App::Key::L, App::KeyModifier::Alt }, [](Display& display) {
        display.toggle_show_line_numbers();
    });

    register_display_keyboard_action("Toggle Word Wrap Enabled", { App::Key::Z, App::KeyModifier::Alt }, [](Display& display) {
        display.toggle_word_wrap_enabled();
    });

    register_display_keyboard_action("Open File", { App::Key::O, App::KeyModifier::Control }, [](Display& display) {
        display.this_widget().start_coroutine(display.do_open_prompt());
    });
    register_display_keyboard_action("Close Display", { App::Key::Q, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        display.this_widget().start_coroutine(document.quit(display));
    });

    register_display_keyboard_action("Save", { App::Key::S, App::KeyModifier::Control }, [](Display& display) {
        auto& document = *display.document();
        display.this_widget().start_coroutine(document.save(display));
    });
}
}
