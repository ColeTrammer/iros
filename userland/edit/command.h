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

private:
    Document::StateSnapshot m_snapshot;
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
    InsertCommand(Document& document, char c);
    virtual ~InsertCommand();

    virtual bool execute() override;
    virtual void undo() override;

private:
    char m_char { 0 };
};

class DeleteCommand final : public SnapshotBackedCommand {
public:
    DeleteCommand(Document& document, DeleteCharMode mode);
    virtual ~DeleteCommand();

    virtual bool execute() override;

private:
    DeleteCharMode m_mode { DeleteCharMode::Delete };
};

class SplitLineCommand final : public DeltaBackedCommand {
public:
    SplitLineCommand(Document& document);
    virtual ~SplitLineCommand();

    virtual bool execute() override;
    virtual void undo() override;
};