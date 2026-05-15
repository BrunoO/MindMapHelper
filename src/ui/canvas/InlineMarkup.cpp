#include "ui/canvas/InlineMarkup.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <optional>

namespace mind_map::ui::canvas {

namespace {

std::optional<InlineMarkupFonts>& FontRegistry() {
  static std::optional<InlineMarkupFonts> instance;
  return instance;
}

// If any style flag is still open at EOF the label has an unmatched opener.
// Emit the raw label as a single plain-text span so markers are visible rather
// than silently consumed. Extracted to keep ParseMarkup under the complexity limit.
void ReconcileUnmatched(std::vector<MarkupSpan>& spans, std::string_view label,  // NOLINT(readability-identifier-naming,cppcoreguidelines-avoid-non-const-global-variables)
                        bool bold, bool italic, bool code, bool strikethrough) {
  if (bold || italic || code || strikethrough) {
    spans.clear();
    spans.push_back(MarkupSpan{std::string{label}});
  }
}

// Handles one character inside an open code span.  Returns true when code mode
// consumed the character (caller should `continue`), false when not in code mode.
// Extracted from ParseMarkup to keep its cognitive complexity under the limit.
bool HandleCodeSpanChar(std::string_view label, size_t& i,  // NOLINT(readability-identifier-naming,cppcoreguidelines-avoid-non-const-global-variables)
                         std::string& buf, std::vector<MarkupSpan>& spans,
                         bool& code, bool bold, bool italic, bool strikethrough) {
  if (!code) { return false; }
  if (label[i] == '`') {
    if (!buf.empty()) {
      spans.push_back({std::move(buf), {}, bold, italic, true, strikethrough});
      buf.clear();
    }
    code = false;
  } else {
    buf += label[i];
  }
  ++i;
  return true;
}

// Tries to parse [text](url) starting at label[i].  On success advances i past
// the closing ')' and returns the span; on failure leaves i unchanged.
std::optional<MarkupSpan> ParseLinkSpanAt(  // NOLINT(readability-identifier-naming,cppcoreguidelines-avoid-non-const-global-variables)
    std::string_view label, size_t& i, bool bold, bool italic, bool strikethrough) {
  const size_t n = label.size();
  size_t j = i + 1;
  while (j < n && label[j] != ']') { ++j; }
  if (j >= n || j + 1 >= n || label[j + 1] != '(') { return std::nullopt; }
  size_t k = j + 2;
  while (k < n && label[k] != ')') { ++k; }
  if (k >= n) { return std::nullopt; }
  MarkupSpan span;
  span.text_ = std::string{label.substr(i + 1, j - i - 1)};
  span.url_  = std::string{label.substr(j + 2, k - j - 2)};
  span.bold_          = bold;
  span.italic_        = italic;
  span.strikethrough_ = strikethrough;
  i = k + 1;
  return span;
}

bool IsDoubleSquiggle(std::string_view sv, size_t i) {  // NOLINT(readability-identifier-naming,cppcoreguidelines-avoid-non-const-global-variables)
  return sv[i] == '~' && i + 1 < sv.size() && sv[i + 1] == '~';
}

}  // namespace

void InitInlineMarkupFonts(InlineMarkupFonts fonts) { FontRegistry() = fonts; }

const InlineMarkupFonts& GetInlineMarkupFonts() {
  auto& reg = FontRegistry();
  if (!reg.has_value()) {
    assert(false && "InitInlineMarkupFonts must be called before rendering markup");
    std::abort();
  }
  return *reg;
}

bool ContainsMarkup(std::string_view label) {
  return std::any_of(label.begin(), label.end(), [](char c) {  // NOLINT(llvm-use-ranges)
    return c == '*' || c == '`' || c == '~' || c == '\\' || c == '[';
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
      spans.push_back({std::move(buf), {}, bold, italic, code, strikethrough});
      buf.clear();
    }
  };

  size_t i = 0;
  const size_t n = label.size();
  while (i < n) {
    // Inside a code span only the closing backtick is special.
    if (HandleCodeSpanChar(label, i, buf, spans, code, bold, italic, strikethrough)) { continue; }

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
    if (IsDoubleSquiggle(label, i)) {
      flush(); strikethrough = !strikethrough; i += 2; continue;
    }

    // [text](url) hyperlink
    if (label[i] == '[') {
      flush();
      if (auto lnk = ParseLinkSpanAt(label, i, bold, italic, strikethrough)) {
        spans.push_back(std::move(*lnk));
        continue;
      }
      buf += label[i++];
      continue;
    }

    buf += label[i++];
  }
  flush();
  ReconcileUnmatched(spans, label, bold, italic, code, strikethrough);
  return spans;
}

std::string InjectMarkupLinks(std::string_view text) {
  std::string result;
  result.reserve(text.size());
  size_t i = 0;
  const size_t n = text.size();
  while (i < n) {
    const bool is_http  = (n - i >= 7U) && text.substr(i, 7) == "http://";
    const bool is_https = (n - i >= 8U) && text.substr(i, 8) == "https://";
    // Skip if already inside a [text](url) construct — detected by preceding '('.
    const bool already_wrapped = !result.empty() && result.back() == '(';
    if ((is_http || is_https) && !already_wrapped) {
      size_t j = i;
      while (j < n && text[j] != ' ' && text[j] != '\n' && text[j] != '\t'
             && text[j] != ')' && text[j] != ']' && text[j] != '>') {
        ++j;
      }
      const std::string_view url = text.substr(i, j - i);
      result += '[';
      result += url;
      result += "](";
      result += url;
      result += ')';
      i = j;
    } else {
      result += text[i++];
    }
  }
  return result;
}

}  // namespace mind_map::ui::canvas
