#pragma once

#include "ui/canvas/InlineMarkup.h"

#include "imgui.h"

#include <string>
#include <vector>

namespace mind_map::ui::canvas {

// Measures the bounding size of the rendered spans in world units — the same coordinate
// system as NodeHalfExtentForLabel / ImGui::CalcTextSize.  wrap_width is in world units.
// Must be called within a valid ImGui frame.
[[nodiscard]] ImVec2 MeasureMarkup(const std::vector<MarkupSpan>& spans, float wrap_width);

// Renders spans to draw_list.  top_left is in screen coordinates.
// font_size and wrap_width are screen units (world × zoom).
void DrawMarkup(ImDrawList* draw_list, ImVec2 top_left,
                float font_size, float wrap_width,
                ImU32 color, const std::vector<MarkupSpan>& spans);

// One word-level bounding box for a link span.  Multiple entries may share a url_
// when a link wraps across lines.  rect_ min/max use the same coordinate system as
// the top_left and font_size passed to CollectLinkHits.
struct LinkHit {
  ImVec2      min_;
  ImVec2      max_;
  std::string url_;
};

// Returns per-word bounding boxes for every link span in the laid-out label.
// Pass atlas_size (ImGui::GetFontSize()) + world-space top_left + wrap_width for
// hit testing against world coordinates; or screen-space equivalents for screen coords.
[[nodiscard]] std::vector<LinkHit> CollectLinkHits(
    const std::vector<MarkupSpan>& spans,
    ImVec2 top_left, float font_size, float wrap_width);

}  // namespace mind_map::ui::canvas
