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

    virtual void redo(MultiCursor& cursors) final;
    virtual bool execute(MultiCursor& cursors) final;
    virtual void undo(MultiCursor& cursors) final;

    virtual bool do_execute(MultiCursor& cursors) = 0;
    virtual void do_undo(MultiCursor& cursors) = 0;

    const Document::StateSnapshot& start_snapshot() const { return m_start_snapshot; }
    const Document::StateSnapshot& end_snapshot() const { return m_end_snapshot; }

    const String& selection_text(int index) const { return m_selection_texts[index]; }

private:
    Document::StateSnapshot m_start_snapshot;
    Document::StateSnapshot m_end_snapshot;
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

    virtual bool do_execute(MultiCursor& cursor) override;
    virtual void do_undo(MultiCursor& cursor) override;

    static void do_insert(Document& document, MultiCursor& cursors, int cursor_index, char c);
    static void do_insert(Document& document, MultiCursor& cursors, int cursor_index, const String& string);

private:
    String m_text;
};

class DeleteCommand final : public DeltaBackedCommand {
public:
    DeleteCommand(Document& document, DeleteCharMode mode);
    virtual ~DeleteCommand();

    virtual bool do_execute(MultiCursor& cursor) override;
    virtual void do_undo(MultiCursor& cursor) override;

private:
    DeleteCharMode m_mode { DeleteCharMode::Delete };
    Vector<char> m_deleted_chars;
};

class DeleteLineCommand final : public DeltaBackedCommand {
public:
    DeleteLineCommand(Document& document);
    virtual ~DeleteLineCommand();

    virtual bool do_execute(MultiCursor& cursor) override;
    virtual void do_undo(MultiCursor& cursor) override;

private:
    Vector<Line> m_saved_lines;
    bool m_document_was_empty { false };
};

class InsertLineCommand final : public DeltaBackedCommand {
public:
    InsertLineCommand(Document& document, String text);
    virtual ~InsertLineCommand() override;

    virtual bool do_execute(MultiCursor& cursor) override;
    virtual void do_undo(MultiCursor& cursor) override;

private:
    String m_text;
};

class SwapLinesCommand final : public DeltaBackedCommand {
public:
    SwapLinesCommand(Document& document, SwapDirection direction);
    virtual ~SwapLinesCommand() override;

    virtual bool do_execute(MultiCursor& cursor) override;
    virtual void do_undo(MultiCursor& cursor) override;

private:
    bool do_swap(Cursor& cursor, SwapDirection direction);

    SwapDirection m_direction { SwapDirection::Down };
};
}
