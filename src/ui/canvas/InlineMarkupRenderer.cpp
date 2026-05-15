#include "ui/canvas/InlineMarkupRenderer.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <string_view>

namespace mind_map::ui::canvas {

namespace {

constexpr ImU32  kCodeBgColor   = IM_COL32(55,  60,  80, 220);   // NOLINT(hicpp-signed-bitwise)
constexpr ImU32  kCodeTextColor = IM_COL32(180, 210, 255, 255);   // NOLINT(hicpp-signed-bitwise)
constexpr float  kCodeBgPadX    = 2.0F;
constexpr float  kCodeBgRadius  = 2.0F;

ImFont* SelectFont(const MarkupSpan& span) {
  if (span.code_)                        { return nullptr; }  // regular + bg rect
  const InlineMarkupFonts& f = GetInlineMarkupFonts();
  if (span.bold_ && span.italic_)        { return f.bold_italic_; }
  if (span.bold_)                        { return f.bold_; }
  if (span.italic_)                      { return f.italic_; }
  return nullptr;  // regular — AddText / CalcTextSizeA treat nullptr as current default
}

float MeasureWord(ImFont* font, float font_size, const char* begin, const char* end) {
  ImFont* const active = (font != nullptr) ? font : ImGui::GetFont();
  return active->CalcTextSizeA(font_size, FLT_MAX, 0.0F, begin, end).x;
}

// Word-wrapping layout engine shared by MeasureMarkup and DrawMarkup.
// Iterates each span's text token by token (words, spaces, newlines) and calls
// place(span, font, word_begin, word_end, word_width_px, offset_from_top_left).
// Returns the bounding size of the laid-out content in the same unit as font_size / wrap_width.
template <typename PlaceFn>
ImVec2 LayoutSpans(const std::vector<MarkupSpan>& spans, float font_size,
                   float wrap_width, PlaceFn place) {
  float x = 0.0F;
  float y = 0.0F;
  float max_x = 0.0F;

  for (const MarkupSpan& span : spans) {
    ImFont* const font      = SelectFont(span);
    const float   space_w   = MeasureWord(font, font_size, " ", nullptr);
    const std::string_view sv{span.text_};
    size_t i = 0;
    while (i < sv.size()) {
      if (sv[i] == '\n') {
        y += font_size;
        x = 0.0F;
        ++i;
        continue;
      }
      if (sv[i] == ' ') {
        x += (x > 0.0F) ? space_w : 0.0F;
        ++i;
        continue;
      }
      // Collect one word (non-whitespace run).
      const size_t j_start = i;
      while (i < sv.size() && sv[i] != ' ' && sv[i] != '\n') { ++i; }
      const char* wb = sv.data() + j_start;
      const char* we = sv.data() + i;
      const float word_w = MeasureWord(font, font_size, wb, we);
      if (x + word_w > wrap_width && x > 0.0F) {
        y += font_size;
        x = 0.0F;
      }
      place(span, font, wb, we, word_w, ImVec2{x, y});
      x += word_w;
      max_x = (std::max)(x, max_x);
    }
  }

  return {(std::min)(max_x, wrap_width), y + font_size};
}

}  // namespace

ImVec2 MeasureMarkup(const std::vector<MarkupSpan>& spans, float wrap_width) {
  const float atlas_size = ImGui::GetFontSize();
  return LayoutSpans(spans, atlas_size, wrap_width,
                     [](const MarkupSpan&, ImFont*, const char*, const char*, float, ImVec2) { /* measure only — no drawing */ });
}

void DrawMarkup(ImDrawList* draw_list, ImVec2 top_left,
                float font_size, float wrap_width,
                ImU32 color, const std::vector<MarkupSpan>& spans) {
  assert(draw_list != nullptr);
  LayoutSpans(spans, font_size, wrap_width,
    [draw_list, top_left, font_size, color](const MarkupSpan& span, ImFont* font, const char* wb, const char* we,
        float word_w, ImVec2 off) {
      const ImVec2 pos{top_left.x + off.x, top_left.y + off.y};
      if (span.code_) {
        draw_list->AddRectFilled({pos.x - kCodeBgPadX, pos.y},
                                 {pos.x + word_w + kCodeBgPadX, pos.y + font_size},
                                 kCodeBgColor, kCodeBgRadius);
        draw_list->AddText(font, font_size, pos, kCodeTextColor, wb, we);
      } else {
        draw_list->AddText(font, font_size, pos, color, wb, we);
      }
      if (span.strikethrough_) {
        const float mid_y = pos.y + font_size * 0.5F;
        draw_list->AddLine({pos.x, mid_y}, {pos.x + word_w, mid_y}, color, 1.0F);
      }
    });
}

}  // namespace mind_map::ui::canvas
