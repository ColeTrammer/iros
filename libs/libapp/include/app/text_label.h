#pragma once

#include <app/widget.h>
#include <liim/string.h>

namespace App {

class TextLabel : public Widget {
public:
    template<typename... Args>
    static SharedPtr<TextLabel> create(SharedPtr<Object> parent, Args... args) {
        auto ret = SharedPtr<TextLabel>(new TextLabel(forward<Args>(args)...));
        if (parent) {
            parent->add_child(ret);
        }
        return ret;
    }

    TextLabel(String text) : m_text(move(text)) {}

    const String& text() const { return m_text; }
    void set_text(String text) { m_text = move(text); }

protected:
    virtual void render() override;

private:
    String m_text;
};

}
