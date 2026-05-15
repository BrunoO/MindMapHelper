#include "ui/UiCommandDispatcher.h"

#include "app/DocumentSessionService.h"
#include "core/Base64.h"
#include "ui/UiState.h"
#include "ui/canvas/ClipboardImage.h"
#include "ui/canvas/MindMapCanvasTreeNav.h"
#include "ui/commands/CollapseNodeCommand.h"
#include "ui/commands/CommandHistory.h"
#include "ui/commands/DeleteNodeCommand.h"
#include "ui/commands/InsertChildNodeCommand.h"
#include "ui/commands/PasteImageCommand.h"
#include "ui/commands/PasteTextCommand.h"

#include "imgui.h"

#include <algorithm>
#include <memory>

namespace mind_map::ui {

namespace {

constexpr float kMinZoom = 0.35F;
constexpr float kMaxZoom = 3.0F;
constexpr float kZoomStep = 0.1F;

void CenterCanvasOnNode(UiState& state, size_t idx) {
  const ImVec2 world = state.canvas_.GetNodeWorldPos(idx);
  state.pan_px_ = {
      state.canvas_sz_.x * 0.5F - world.x * state.zoom_,
      state.canvas_sz_.y * 0.5F - world.y * state.zoom_,
  };
}

void NavigateTo(UiState& state, std::optional<size_t> target) {
  if (!target.has_value()) { return; }
  state.canvas_.SelectNode(target);
  CenterCanvasOnNode(state, *target);
}

// Handles the five commands that all require a current selection: the four
// navigation directions and RenameNode. Extracted to keep Dispatch under the
// cognitive-complexity threshold.
void DispatchSelectionCommand(UiCommandId command, UiState& state) {
  const auto sel = state.canvas_.GetSelectedNode();
  if (!sel) { return; }
  switch (command) {  // NOSONAR(cpp:S6177)
    case UiCommandId::NavigateParent:      NavigateTo(state, canvas::GetParentOf(state.canvas_, *sel));           return;
    case UiCommandId::NavigateFirstChild:  NavigateTo(state, canvas::GetFirstActiveChildOf(state.canvas_, *sel)); return;
    case UiCommandId::NavigatePrevSibling: NavigateTo(state, canvas::GetPrevSiblingOf(state.canvas_, *sel));      return;
    case UiCommandId::NavigateNextSibling: NavigateTo(state, canvas::GetNextSiblingOf(state.canvas_, *sel));      return;
    case UiCommandId::RenameNode:          state.canvas_.BeginEditing(*sel);                                      return;
    default: return;
  }
}

}  // namespace

UiCommandDispatcher::UiCommandDispatcher(commands::CommandHistory& history) : history_(history) {}

void UiCommandDispatcher::Dispatch(UiCommandId command, UiState& state,
                                   mind_map::app::DocumentSessionService& session) const {
  switch (command) {  // NOSONAR(cpp:S6177)
    case UiCommandId::ResetLayout:
      state.canvas_.Reset();
      session.MarkDirty();
      return;
    case UiCommandId::ZoomIn:  // NOLINT(bugprone-branch-clone)
      state.zoom_ = std::clamp(state.zoom_ + kZoomStep, kMinZoom, kMaxZoom);
      return;
    case UiCommandId::ZoomOut:
      state.zoom_ = std::clamp(state.zoom_ - kZoomStep, kMinZoom, kMaxZoom);
      return;
    case UiCommandId::ResetView:
      state.zoom_ = 1.0F;
      state.pan_px_ = {kInitialPanX, kInitialPanY};
      return;
    case UiCommandId::ToggleStatusBar:
      state.show_status_bar_ = !state.show_status_bar_;
      return;
    case UiCommandId::DeleteNode: {
      if (const auto sel = state.canvas_.GetSelectedChildForBranchStyle(); sel.has_value() && *sel > 0U) {  // root (index 0) cannot be deleted
        history_.Push(std::make_unique<commands::DeleteNodeCommand>(state.canvas_, *sel));
        session.MarkDirty();
      }
      return;
    }
    case UiCommandId::InsertChildNode: {
      const auto sel = state.canvas_.GetSelectedChildForBranchStyle();
      const size_t parent = (sel && *sel > 0U) ? *sel : 0U;  // default to root when nothing selected
      history_.Push(std::make_unique<commands::InsertChildNodeCommand>(state.canvas_, parent));
      session.MarkDirty();
      return;
    }
    case UiCommandId::Undo:
      history_.Undo();
      session.MarkDirty();
      return;
    case UiCommandId::Redo:
      history_.Redo();
      session.MarkDirty();
      return;
    case UiCommandId::Paste: {
      const auto sel = state.canvas_.GetSelectedNode();
      if (!sel.has_value()) { return; }
      if (const auto png = GetClipboardImagePng(); png.has_value()) {
        history_.Push(std::make_unique<commands::PasteImageCommand>(
            state.canvas_, *sel, mind_map::core::Base64Encode(*png)));
        session.MarkDirty();
        return;
      }
      if (const char* const text = ImGui::GetClipboardText(); text != nullptr && text[0] != '\0') {
        history_.Push(std::make_unique<commands::PasteTextCommand>(
            state.canvas_, *sel, text));
        session.MarkDirty();
      }
      return;
    }
    case UiCommandId::ToggleCollapsed: {
      const auto sel = state.canvas_.GetSelectedNode();
      if (!sel.has_value() || !state.canvas_.NodeHasChildren(*sel)) { return; }
      const bool collapsing = !state.canvas_.IsCollapsed(*sel);
      history_.Push(std::make_unique<commands::CollapseNodeCommand>(state.canvas_, *sel, collapsing));
      session.MarkDirty();
      return;
    }
    case UiCommandId::NavigateParent:
    case UiCommandId::NavigateFirstChild:
    case UiCommandId::NavigatePrevSibling:
    case UiCommandId::NavigateNextSibling:
    case UiCommandId::RenameNode:
      DispatchSelectionCommand(command, state);
      return;
  }
}

}  // namespace mind_map::ui
