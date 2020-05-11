#pragma once

#include "document.h"

class Command {
public:
    Command(Document& document);
    virtual ~Command();

    Document& document() { return m_document; }
    const Document& document() const { return m_document; }

    virtual bool execute() = 0;
    virtual void undo() = 0;
    virtual void redo() = 0;

private:
    Document& m_document;
};

class DeltaBackedCommand : public Command {
public:
    DeltaBackedCommand(Document& document);
    virtual ~DeltaBackedCommand() override;

    virtual void redo() override;

    const Document::StateSnapshot& state_snapshot() const { return m_snapshot; }

    const String& selection_text() const { return m_selection_text; }

private:
    Document::StateSnapshot m_snapshot;
    String m_selection_text;
};

class SnapshotBackedCommand : public Command {
public:
    SnapshotBackedCommand(Document& document);
    virtual ~SnapshotBackedCommand() override;

    virtual void undo() override;
    virtual void redo() override;

    const Document::Snapshot& snapshot() const { return m_snapshot; }

private:
    Document::Snapshot m_snapshot;
};

class InsertCommand final : public DeltaBackedCommand {
public:
    InsertCommand(Document& document, String string);
    virtual ~InsertCommand();

    virtual bool execute() override;
    virtual void undo() override;

    static void do_insert(Document& document, char c);
    static void do_insert(Document& document, const String& string);

private:
    String m_text;
};

class DeleteCommand final : public DeltaBackedCommand {
public:
    DeleteCommand(Document& document, DeleteCharMode mode, bool should_clear_selection = false);
    virtual ~DeleteCommand();

    virtual bool execute() override;
    virtual void undo() override;

private:
    DeleteCharMode m_mode { DeleteCharMode::Delete };
    bool m_should_clear_selection { false };
    char m_deleted_char { 0 };
    int m_end_line { 0 };
    int m_end_index { 0 };
};

class DeleteLineCommand final : public DeltaBackedCommand {
public:
    DeleteLineCommand(Document& document);
    virtual ~DeleteLineCommand();

    virtual bool execute() override;
    virtual void undo() override;

private:
    Line m_saved_line;
    bool m_document_was_empty { false };
};

class InsertLineCommand final : public DeltaBackedCommand {
public:
    InsertLineCommand(Document& document, String text);
    virtual ~InsertLineCommand() override;

    virtual bool execute() override;
    virtual void undo() override;

private:
    String m_text;
};
