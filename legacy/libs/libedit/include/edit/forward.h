#pragma once

#include <stddef.h>

namespace Edit {
class AbsolutePosition;
class CharacterMetadata;
class Command;
class Cursor;
class Display;
class DisplayBridge;
class DisplayPosition;
class Document;
class Line;
class LineRenderer;
class MatchedSuggestion;
class MultiCursor;
class Selection;
class Suggestion;
class Suggestions;
class TextIndex;
class TextRangeCollection;
class TextRangeCollectionIterator;

template<size_t N>
class TextRangeCombinerIterator;

class TextRange;
enum class DocumentType;
struct LineSplitResult;
enum class PositionRangeType;
struct PositionRange;
class RelativePosition;
struct RenderedLine;

using DocumentTextRangeIterator = TextRangeCombinerIterator<4>;
}
