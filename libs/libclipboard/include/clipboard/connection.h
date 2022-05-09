#pragma once

#include <liim/option.h>
#include <liim/string.h>

namespace Clipboard {

class Connection {
public:
    static Connection& the();

    bool set_clipboard_contents_to_text(const String& text);
    Option<String> get_clipboard_contents_as_text();
};

}
