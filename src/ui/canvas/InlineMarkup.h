#pragma once

#include "imgui.h"

#include <string>
#include <string_view>
#include <vector>

namespace mind_map::ui::canvas {

// A contiguous run of text sharing one visual style.
struct MarkupSpan {
  std::string text_;           // display text — escape sequences resolved, no markup syntax
  std::string url_;            // non-empty → span is a clickable hyperlink
  bool bold_          = false;
  bool italic_        = false;
  bool code_          = false;  // rendered with background tint; no font switch
  bool strikethrough_ = false;
};

// Returns true if label contains any markup syntax.  Used as a fast path to skip
// parsing for plain-text labels (the common case).
[[nodiscard]] bool ContainsMarkup(std::string_view label);

// Parses inline markup into styled spans.
// Supported: **bold**  *italic*  ***bold+italic***  `code`  ~~strikethrough~~  \<escape>
//            [text](url) — clickable hyperlink; url stored in MarkupSpan::url_
// Code spans are raw: style markers inside backticks are treated as literal text.
[[nodiscard]] std::vector<MarkupSpan> ParseMarkup(std::string_view label);

// Scans plain text for bare http:// / https:// URLs and wraps each one as [url](url)
// so pasted text automatically becomes clickable markup.  Already-wrapped links are
// left unchanged (detected by a preceding '(' in the accumulated output).
[[nodiscard]] std::string InjectMarkupLinks(std::string_view text);

// Font variants for the markup renderer, loaded at atlas-build time alongside Inter-Regular.
// bold / italic / bold_italic use FreeType synthetic flags — no extra .ttf files are needed.
// code spans use the default font (nullptr) with a coloured background rect.
struct InlineMarkupFonts {
  ImFont* bold_        = nullptr;
  ImFont* italic_      = nullptr;
  ImFont* bold_italic_ = nullptr;
};

// Called once from AppMain after the font atlas is built.
void InitInlineMarkupFonts(InlineMarkupFonts fonts);
[[nodiscard]] const InlineMarkupFonts& GetInlineMarkupFonts();

}  // namespace mind_map::ui::canvas
