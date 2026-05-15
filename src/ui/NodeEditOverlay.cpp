#include "ui/NodeEditOverlay.h"

#include "app/DocumentSessionService.h"
#include "ui/MindMapCanvasView.h"
#include "ui/canvas/CanvasMath.h"
#include "ui/canvas/MindMapCanvasEditState.h"
#include "ui/canvas/MindMapCanvasNodeMutators.h"
#include "ui/canvas/NodeGeometry.h"
#include "ui/commands/CommandHistory.h"
#include "ui/commands/RenameNodeCommand.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <memory>

namespace mind_map::ui {

namespace {

constexpr ImU32 kEditFrameBg     = IM_COL32(42,  42,  55, 255);   // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kEditBorderColor = IM_COL32(120, 180, 255, 200);  // NOLINT(hicpp-signed-bitwise)
constexpr float kEditBorderThick = 1.5F;
constexpr float kMinBoxWidth     = 80.0F;
constexpr float kMinBoxHeight    = 28.0F;

void CommitEdit(MindMapCanvasView& canvas,
                mind_map::app::DocumentSessionService& session,
                commands::CommandHistory& history) {
  const auto editing = canvas::GetEditingNode(canvas);
  if (!editing.has_value()) { return; }
  const size_t idx  = *editing;
  std::string old_label = canvas::GetNodeLabel(canvas, idx);
  std::string new_label = canvas::GetEditBuffer(canvas);
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
  if (!canvas::IsEditing(canvas)) { return; }

  const auto editing = canvas::GetEditingNode(canvas);
  if (!editing.has_value()) { return; }
  const size_t idx = *editing;
  const ImVec2 world_pos = canvas.GetNodeWorldPos(idx);

  const float inner_w_world  = mind_map::canvas::kNodeMaxLabelWidth;
  const float pad_screen     = mind_map::canvas::kNodePad * ctx.zoom_;
  const float inner_w_screen = inner_w_world * ctx.zoom_;

  // Size the box to the current buffer text, growing downward.
  std::string& buf = canvas::GetEditBuffer(canvas);
  const ImVec2 text_sz_base = ImGui::CalcTextSize(buf.c_str(), nullptr, false, inner_w_world);
  const float box_w = (std::max)(inner_w_screen + 2.0F * pad_screen, kMinBoxWidth);
  const float box_h = (std::max)(text_sz_base.y * ctx.zoom_ + 2.0F * pad_screen, kMinBoxHeight);

  const ImVec2 centre = mind_map::canvas::WorldToScreen(world_pos, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 box_tl = {centre.x - box_w * 0.5F, centre.y - box_h * 0.5F};

  ctx.draw_list_->AddRectFilled(box_tl, {box_tl.x + box_w, box_tl.y + box_h},
                                kEditFrameBg, mind_map::canvas::kNodeCornerRadiusWorld);
  ctx.draw_list_->AddRect(box_tl, {box_tl.x + box_w, box_tl.y + box_h},
                          kEditBorderColor, mind_map::canvas::kNodeCornerRadiusWorld,
                          0, kEditBorderThick);

  ImGui::SetCursorScreenPos(box_tl);
  ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_screen, pad_screen));

  // Enter inserts a newline (multiline default); Shift+Enter confirms and exits.
  constexpr ImGuiInputTextFlags kFlags = ImGuiInputTextFlags_AutoSelectAll;

  ImGui::InputTextMultiline("##node_edit", &buf, ImVec2(box_w, box_h), kFlags);

  ImGui::PopStyleVar();
  ImGui::PopStyleColor(3);

  // Request keyboard focus every frame until the widget is active.
  if (!ImGui::IsItemActive()) {
    ImGui::SetKeyboardFocusHere(-1);
  }

  if (ImGui::IsKeyPressed(ImGuiKey_Escape, /*repeat=*/false)) {
    canvas.CancelEditing();
    return;
  }

  // Shift+Enter confirms. The widget already inserted a '\n' for the Enter key,
  // so strip it from the buffer before committing.
  const bool shift_enter = ImGui::IsKeyPressed(ImGuiKey_Enter, /*repeat=*/false)
                           && ImGui::GetIO().KeyShift;
  if (shift_enter) {
    if (!buf.empty() && buf.back() == '\n') { buf.pop_back(); }
    CommitEdit(canvas, session, history);
    return;
  }

  // Clicking outside commits — but not on the same double-click frame that opened editing,
  // because IsMouseClicked fires on that frame too and IsItemHovered is not yet true.
  const bool opened_this_frame = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
  if (!opened_this_frame && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsItemHovered()) {
    CommitEdit(canvas, session, history);
  }
}

}  // namespace mind_map::ui
