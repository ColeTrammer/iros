#pragma once

#include <edit/document.h>
#include <edit/text_index.h>

namespace Edit {
class Command {
public:
    Command(Document& document);
    virtual ~Command();

    Document& document() { return m_document; }
    const Document& document() const { return m_document; }

    virtual bool execute(MultiCursor& cursor) = 0;
    virtual void undo(MultiCursor& cursor) = 0;
    virtual void redo(MultiCursor& cursor) = 0;

private:
    Document& m_document;
};

class DeltaBackedCommand : public Command {
public:
    DeltaBackedCommand(Document& document);
    virtual ~DeltaBackedCommand() override;

    virtual void redo(MultiCursor& cursor) override;

    const Document::StateSnapshot& state_snapshot() const { return m_snapshot; }

    const String& selection_text(int index) const { return m_selection_texts[index]; }

private:
    Document::StateSnapshot m_snapshot;
    Vector<String> m_selection_texts;
};

class SnapshotBackedCommand : public Command {
public:
    SnapshotBackedCommand(Document& document);
    virtual ~SnapshotBackedCommand() override;

    virtual void undo(MultiCursor& cursor) override;
    virtual void redo(MultiCursor& cursor) override;

    const Document::Snapshot& snapshot() const { return m_snapshot; }

private:
    Document::Snapshot m_snapshot;
};

class InsertCommand final : public DeltaBackedCommand {
public:
    InsertCommand(Document& document, String string);
    virtual ~InsertCommand();

    virtual bool execute(MultiCursor& cursor) override;
    virtual void undo(MultiCursor& cursor) override;

    static void do_insert(Document& document, Cursor& cursor, char c);
    static void do_insert(Document& document, Cursor& cursor, const String& string);

private:
    String m_text;
};

class DeleteCommand final : public DeltaBackedCommand {
public:
    DeleteCommand(Document& document, DeleteCharMode mode, bool should_clear_selection = false);
    virtual ~DeleteCommand();

    virtual bool execute(MultiCursor& cursor) override;
    virtual void undo(MultiCursor& cursor) override;

private:
    DeleteCharMode m_mode { DeleteCharMode::Delete };
    bool m_should_clear_selection { false };
    Vector<char> m_deleted_chars;
    Vector<TextIndex> m_end_indices;
};

class DeleteLineCommand final : public DeltaBackedCommand {
public:
    DeleteLineCommand(Document& document);
    virtual ~DeleteLineCommand();

    virtual bool execute(MultiCursor& cursor) override;
    virtual void undo(MultiCursor& cursor) override;

private:
    Vector<Line> m_saved_lines;
    bool m_document_was_empty { false };
};

class InsertLineCommand final : public DeltaBackedCommand {
public:
    InsertLineCommand(Document& document, String text);
    virtual ~InsertLineCommand() override;

    virtual bool execute(MultiCursor& cursor) override;
    virtual void undo(MultiCursor& cursor) override;

private:
    String m_text;
};

class SwapLinesCommand final : public DeltaBackedCommand {
public:
    SwapLinesCommand(Document& document, SwapDirection direction);
    virtual ~SwapLinesCommand() override;

    virtual bool execute(MultiCursor& cursor) override;
    virtual void undo(MultiCursor& cursor) override;

private:
    bool do_swap(Cursor& cursor, SwapDirection direction);

    SwapDirection m_direction { SwapDirection::Down };
    TextIndex m_end;
    Selection m_end_selection;
};
}
