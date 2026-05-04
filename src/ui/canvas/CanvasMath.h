#pragma once

#include "imgui.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace mind_map::canvas {

constexpr ImU32 kGridMajorColor = IM_COL32(60, 60, 72, 255);   // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kGridMinorColor = IM_COL32(45, 45, 55, 255);   // NOLINT(hicpp-signed-bitwise)

[[nodiscard]] inline ImVec2 WorldToScreen(ImVec2 world, ImVec2 canvas_p0, ImVec2 pan_px, float zoom) {
  return {canvas_p0.x + pan_px.x + world.x * zoom, canvas_p0.y + pan_px.y + world.y * zoom};
}

[[nodiscard]] inline ImVec2 ScreenToWorld(ImVec2 screen, ImVec2 canvas_p0, ImVec2 pan_px, float zoom) {
  assert(zoom > 0.0F);
  return {(screen.x - canvas_p0.x - pan_px.x) / zoom, (screen.y - canvas_p0.y - pan_px.y) / zoom};
}

[[nodiscard]] inline bool IsInsideRect(ImVec2 p, ImVec2 rect_min, ImVec2 rect_max) {
  return p.x >= rect_min.x && p.x <= rect_max.x && p.y >= rect_min.y && p.y <= rect_max.y;
}

inline void DrawGrid(ImDrawList* draw_list, ImVec2 canvas_p0, ImVec2 canvas_p1, ImVec2 pan_px, float zoom) {
  assert(draw_list != nullptr);
  constexpr float kMinorStep = 32.0F;
  constexpr int kMajorEvery = 4;

  const ImVec2 world_min = ScreenToWorld(canvas_p0, canvas_p0, pan_px, zoom);
  const ImVec2 world_max = ScreenToWorld(canvas_p1, canvas_p0, pan_px, zoom);

  const int ix0 = static_cast<int>(std::floor(world_min.x / kMinorStep));
  const int ix1 = static_cast<int>(std::ceil(world_max.x / kMinorStep));
  const int iy0 = static_cast<int>(std::floor(world_min.y / kMinorStep));
  const int iy1 = static_cast<int>(std::ceil(world_max.y / kMinorStep));

  for (int ix = ix0; ix <= ix1; ++ix) {
    const float x = static_cast<float>(ix) * kMinorStep;
    const ImVec2 a = WorldToScreen({x, world_min.y}, canvas_p0, pan_px, zoom);
    const ImVec2 b = WorldToScreen({x, world_max.y}, canvas_p0, pan_px, zoom);
    const int mod = ((ix % kMajorEvery) + kMajorEvery) % kMajorEvery;
    const bool major = (mod == 0);
    draw_list->AddLine(a, b, major ? kGridMajorColor : kGridMinorColor, 1.0F);
  }
  for (int iy = iy0; iy <= iy1; ++iy) {
    const float y = static_cast<float>(iy) * kMinorStep;
    const ImVec2 a = WorldToScreen({world_min.x, y}, canvas_p0, pan_px, zoom);
    const ImVec2 b = WorldToScreen({world_max.x, y}, canvas_p0, pan_px, zoom);
    const int mod = ((iy % kMajorEvery) + kMajorEvery) % kMajorEvery;
    const bool major = (mod == 0);
    draw_list->AddLine(a, b, major ? kGridMajorColor : kGridMinorColor, 1.0F);
  }
}

}  // namespace mind_map::canvas
