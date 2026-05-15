#include "ui/NodeEditOverlay.h"

#include "app/DocumentSessionService.h"
#include "ui/MindMapCanvasView.h"
#include "ui/canvas/CanvasMath.h"
#include "ui/canvas/MindMapCanvasNodeMutators.h"
#include "ui/canvas/NodeExtent.h"
#include "ui/canvas/NodeGeometry.h"
#include "ui/commands/CommandHistory.h"
#include "ui/commands/RenameNodeCommand.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <memory>

namespace mind_map::ui {

namespace {

constexpr ImU32 kEditFrameBg     = IM_COL32(42,  42,  55, 255);  // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kEditBorderColor = IM_COL32(120, 180, 255, 200); // NOLINT(hicpp-signed-bitwise)
constexpr float kEditBorderThick = 1.5F;
constexpr float kMinBoxWidth     = 80.0F;   // screen pixels
constexpr float kMinBoxHeight    = 28.0F;   // screen pixels

// Measure the wrapped screen size of the given text at the current edit-box width.
[[nodiscard]] ImVec2 MeasureEditText(const std::string& text, float inner_width_world, float zoom) {
  const ImVec2 base = ImGui::CalcTextSize(text.c_str(), nullptr, false, inner_width_world);
  return {base.x * zoom, base.y * zoom};
}

void CommitEdit(MindMapCanvasView& canvas,
                mind_map::app::DocumentSessionService& session,
                commands::CommandHistory& history) {
  const auto editing = canvas.GetEditingNode();
  if (!editing.has_value()) { return; }
  const size_t idx   = *editing;
  std::string old_label = canvas::GetNodeLabel(canvas, idx);
  std::string new_label = canvas.GetEditBuffer();
  canvas.CancelEditing();
  if (old_label != new_label) {
    history.Push(std::make_unique<commands::RenameNodeCommand>(
        canvas, idx, std::move(old_label), std::move(new_label)));
    session.MarkDirty();
  }
}

}  // namespace

void RenderNodeEditOverlay(const MindMapCanvasRenderContext& ctx,
                           MindMapCanvasView& canvas,
                           mind_map::app::DocumentSessionService& session,
                           commands::CommandHistory& history) {
  if (!canvas.IsEditing()) { return; }

  const auto editing = canvas.GetEditingNode();
  if (!editing.has_value()) { return; }
  const size_t idx = *editing;
  const ImVec2 world_pos = canvas.GetNodeWorldPos(idx);

  // Compute inner wrap width (world units) — capped at kNodeMaxLabelWidth.
  const float inner_w_world = mind_map::canvas::kNodeMaxLabelWidth;
  const float pad_screen    = mind_map::canvas::kNodePad * ctx.zoom_;
  const float inner_w_screen = inner_w_world * ctx.zoom_;

  // Measure current buffer to determine box height.
  const ImVec2 text_sz = MeasureEditText(canvas.GetEditBuffer(), inner_w_world, ctx.zoom_);
  const float box_w = (std::max)(inner_w_screen + 2.0F * pad_screen, kMinBoxWidth);
  const float box_h = (std::max)(text_sz.y + 2.0F * pad_screen, kMinBoxHeight);

  // Centre the box on the node's world position.
  const ImVec2 centre_screen = mind_map::canvas::WorldToScreen(world_pos, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 box_tl = {centre_screen.x - box_w * 0.5F, centre_screen.y - box_h * 0.5F};

  // Draw a highlighted border behind the widget so the editing state is obvious.
  ctx.draw_list_->AddRectFilled(box_tl, {box_tl.x + box_w, box_tl.y + box_h},
                                kEditFrameBg, mind_map::canvas::kNodeCornerRadiusWorld);
  ctx.draw_list_->AddRect(box_tl, {box_tl.x + box_w, box_tl.y + box_h},
                          kEditBorderColor, mind_map::canvas::kNodeCornerRadiusWorld,
                          0, kEditBorderThick);

  // Place the InputTextMultiline widget over the node.
  ImGui::SetCursorScreenPos(box_tl);
  ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,  ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive,   ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_screen, pad_screen));

  constexpr ImGuiInputTextFlags kFlags =
      ImGuiInputTextFlags_AutoSelectAll |
      ImGuiInputTextFlags_EnterReturnsTrue;

  const bool confirmed = ImGui::InputTextMultiline(
      "##node_edit",
      &canvas.GetEditBuffer(),
      ImVec2(box_w, box_h),
      kFlags);

  ImGui::PopStyleVar();
  ImGui::PopStyleColor(3);

  // Focus the widget on the first frame it appears.
  if (ImGui::IsWindowFocused() && !ImGui::IsItemActive()) {
    ImGui::SetKeyboardFocusHere(-1);
  }

  if (confirmed) {
    CommitEdit(canvas, session, history);
    return;
  }

  // Escape cancels without saving.
  if (ImGui::IsKeyPressed(ImGuiKey_Escape, /*repeat=*/false)) {
    canvas.CancelEditing();
    return;
  }

  // Clicking outside the widget commits.
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered()) {
    CommitEdit(canvas, session, history);
  }
}

}  // namespace mind_map::ui
