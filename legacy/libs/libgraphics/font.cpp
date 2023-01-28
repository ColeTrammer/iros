#include <assert.h>
#include <graphics/font.h>
#include <graphics/psf/font.h>
#include <liim/string.h>

SharedPtr<Font> s_default;
SharedPtr<Font> s_bold;

SharedPtr<Font> Font::default_font() {
    if (!s_default) {
        s_default = PSF::Font::try_create_from_path(RESOURCE_ROOT "/usr/share/font.psf");
        assert(s_default);
    }
    return s_default;
}

SharedPtr<Font> Font::bold_font() {
    if (!s_bold) {
        s_bold = PSF::Font::try_create_from_path(RESOURCE_ROOT "/usr/share/bold.psf");
        assert(s_bold);
    }
    return s_bold;
}

Font::Font() {}

Font::~Font() {}
