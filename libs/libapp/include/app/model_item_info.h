#pragma once

#include <graphics/bitmap.h>
#include <graphics/text_align.h>
#include <liim/maybe.h>
#include <liim/string.h>

namespace App {
class ModelItemInfo {
public:
    enum Request {
        Text = 1,
        Bitmap = 2,
        TextAlign = 4,
    };

    void set_text(String text) { m_text = move(text); }
    void set_bitmap(SharedPtr<::Bitmap> bitmap) { m_bitmap = move(bitmap); }
    void set_text_align(::TextAlign text_align) { m_text_align = text_align; }

    const Maybe<String>& text() const { return m_text; }
    SharedPtr<::Bitmap> bitmap() const { return m_bitmap; }
    const Maybe<::TextAlign>& text_align() const { return m_text_align; }

private:
    Maybe<String> m_text;
    SharedPtr<::Bitmap> m_bitmap;
    Maybe<::TextAlign> m_text_align;
};
}
