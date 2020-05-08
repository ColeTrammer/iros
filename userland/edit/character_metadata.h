#pragma once

class CharacterMetadata {
public:
    enum Flags {
        Highlighted = 1,
        Selected = 2,
    };

    CharacterMetadata() {}
    CharacterMetadata(int flags) : m_flags(flags) {}

    bool operator==(const CharacterMetadata& other) const { return this->m_flags == other.m_flags; }
    bool operator!=(const CharacterMetadata& other) const { return !(*this == other); }

    bool highlighted() const { return m_flags & Flags::Highlighted; }
    void set_highlighted(bool b) {
        if (b) {
            m_flags |= Flags::Highlighted;
        } else {
            m_flags &= ~Flags::Highlighted;
        }
    }
    void invert_highlighted() { m_flags ^= Flags::Highlighted; }

    bool selected() const { return m_flags & Flags::Selected; }
    void set_selected(bool b) {
        if (b) {
            m_flags |= Flags::Selected;
        } else {
            m_flags &= ~Flags::Selected;
        }
    }
    void invert_selected() { m_flags ^= Flags::Selected; }

private:
    int m_flags { 0 };
};