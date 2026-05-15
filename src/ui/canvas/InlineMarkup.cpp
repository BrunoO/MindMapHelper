#include "ui/canvas/InlineMarkup.h"

#include <algorithm>

namespace mind_map::ui::canvas {

namespace {
InlineMarkupFonts& FontRegistry() {
  static InlineMarkupFonts instance;
  return instance;
}
}

void InitInlineMarkupFonts(InlineMarkupFonts fonts) { FontRegistry() = fonts; }
const InlineMarkupFonts& GetInlineMarkupFonts()     { return FontRegistry(); }

bool ContainsMarkup(std::string_view label) {
  return std::any_of(label.begin(), label.end(), [](char c) {  // NOLINT(llvm-use-ranges)
    return c == '*' || c == '`' || c == '~' || c == '\\';
  });
}

std::vector<MarkupSpan> ParseMarkup(std::string_view label) {
  std::vector<MarkupSpan> spans;
  std::string buf;
  bool bold          = false;
  bool italic        = false;
  bool code          = false;
  bool strikethrough = false;

  auto flush = [&]() {
    if (!buf.empty()) {
      spans.push_back({std::move(buf), bold, italic, code, strikethrough});
      buf.clear();
    }
  };

  size_t i = 0;
  const size_t n = label.size();
  while (i < n) {
    // Inside a code span only the closing backtick is special.
    if (code) {
      if (label[i] == '`') { flush(); code = false; }
      else                  { buf += label[i]; }
      ++i;
      continue;
    }

    // Backslash escape: emit next char literally.
    if (label[i] == '\\' && i + 1 < n) { buf += label[i + 1]; i += 2; continue; }

    // *** bold+italic — must be checked before ** and *.
    if (i + 2 < n && label[i] == '*' && label[i+1] == '*' && label[i+2] == '*') {
      flush(); bold = !bold; italic = !italic; i += 3; continue;
    }
    // ** bold
    if (i + 1 < n && label[i] == '*' && label[i+1] == '*') {
      flush(); bold = !bold; i += 2; continue;
    }
    // * italic
    if (label[i] == '*') { flush(); italic = !italic; ++i; continue; }

    // ` open code span
    if (label[i] == '`') { flush(); code = true; ++i; continue; }

    // ~~ strikethrough
    if (i + 1 < n && label[i] == '~' && label[i+1] == '~') {
      flush(); strikethrough = !strikethrough; i += 2; continue;
    }

    buf += label[i++];
  }
  flush();
  return spans;
}

}  // namespace mind_map::ui::canvas
