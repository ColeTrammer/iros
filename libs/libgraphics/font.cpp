#include <graphics/font.h>
#include <stdio.h>

bool Font::save_to_file(const String& path) const {
    FILE* file = fopen(path.string(), "w");
    if (!file) {
        return false;
    }

    // PSF font header: 0x36 0x04 <filemode> <fontheight>
    fputc(0x36, file);
    fputc(0x04, file);
    fputc(0, file);
    fputc(16, file);

    for (int i = 0; i < 256; i++) {
        auto bitmap = get_for_character(i);
        bitmap->for_each_storage_part([&](uint8_t byte) {
            fputc(byte, file);
        });
    }

    bool ret = !ferror(file);

    if (fclose(file)) {
        return false;
    }

    return ret;
}
