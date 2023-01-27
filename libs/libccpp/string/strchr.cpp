extern "C" char* strchr(char const* str, int ch) {
    auto needle = (char) ch;
    auto ustr = (unsigned char const*) str;

    if (*ustr == needle) {
        return (char*) ustr;
    } else if (*ustr == '\0') {
        return nullptr;
    }

    do {
        if (*++ustr == needle) {
            return (char*) ustr;
        }
    } while (*ustr != '\0');
    return nullptr;
}