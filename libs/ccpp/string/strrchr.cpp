extern "C" unsigned char const* strrchr(unsigned char const* str, int ch) {
    auto needle = (char) ch;

    unsigned char const* result = nullptr;
    if (*str == needle) {
        result = str;
    }
    if (*str == '\0') {
        return result;
    }

    do {
        if (*++str == needle) {
            result = str;
        }
    } while (*str != '\0');
    return result;
}
