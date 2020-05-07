#pragma once

class CharacterMetadata {
public:
    enum Flags {
        Highlighted,
        Selected,
    };

    CharacterMetadata() {}
    CharacterMetadata(int flags) : m_flags(flags) {}

    bool highlighted() const { return m_flags & Flags::Highlighted; }
    void set_highlighted(bool b) {
        if (b) {
            m_flags |= Flags::Highlighted;
        } else {
            m_flags &= ~Flags::Highlighted;
        }
    }

    bool selected() const { return m_flags & Flags::Selected; }
    void set_selected(bool b) {
        if (b) {
            m_flags |= Flags::Selected;
        } else {
            m_flags &= ~Flags::Selected;
        }
    }

private:
    int m_flags { 0 };
};