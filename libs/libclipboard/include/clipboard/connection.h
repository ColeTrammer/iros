#pragma once

#include <liim/maybe.h>
#include <liim/string.h>

namespace Clipboard {

class Connection {
public:
    static Connection& the();

    bool set_clipboard_contents_to_text(const String& text);
    Maybe<String> get_clipboard_contents_as_text();
};

}
