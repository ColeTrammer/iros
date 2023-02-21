extern "C" unsigned char const* strchr(unsigned char const* str, int ch) {
    auto needle = (char) ch;

    if (*str == needle) {
        return str;
    } else if (*str == '\0') {
        return nullptr;
    }

    do {
        if (*++str == needle) {
            return str;
        }
    } while (*str != '\0');
    return nullptr;
}
