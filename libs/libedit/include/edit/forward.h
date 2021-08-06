#pragma once

#include <stddef.h>

namespace Edit {
class CharacterMetadata;
class Command;
class Cursor;
class Display;
class Document;
class LineRenderer;
class Line;
class MultiCursor;
class Selection;
class Suggestions;
class Suggestion;
class TextIndex;
class TextRangeCollectionIterator;
class TextRangeCollection;

template<size_t N>
class TextRangeCombinerIterator;

class TextRange;
enum class DocumentType;
struct LineSplitResult;
enum class PositionRangeType;
struct PositionRange;
struct Position;
struct RenderedLine;

using DocumentTextRangeIterator = TextRangeCombinerIterator<4>;
}
