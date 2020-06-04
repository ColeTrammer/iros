#include "app_panel.h"

AppPanel::AppPanel(int x, int y, int width, int height) {
    set_rect({ x, y, width, height });
}

AppPanel::~AppPanel() {}

void AppPanel::clear() {}

void AppPanel::set_text_at(int, int, char, CharacterMetadata) {}

void AppPanel::flush() {}

void AppPanel::enter() {};

void AppPanel::send_status_message(String) {}

String AppPanel::prompt(const String&) {
    return "";
}

void AppPanel::enter_search(String) {}

void AppPanel::notify_line_count_changed() {}

void AppPanel::set_clipboard_contents(String, bool) {}

String AppPanel::clipboard_contents(bool&) const {
    return "";
}

void AppPanel::set_cursor(int, int) {}

void AppPanel::do_open_prompt() {}
