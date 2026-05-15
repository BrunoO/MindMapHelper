#pragma once

#include "ui/canvas/InlineMarkup.h"

#include "imgui.h"

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

}  // namespace mind_map::ui::canvas
