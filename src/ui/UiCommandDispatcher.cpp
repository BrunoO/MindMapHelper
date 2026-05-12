#include "ui/UiCommandDispatcher.h"

#include "app/DocumentSessionService.h"
#include "ui/UiState.h"
#include "ui/commands/CommandHistory.h"
#include "ui/commands/DeleteNodeCommand.h"
#include "ui/commands/InsertChildNodeCommand.h"

#include <algorithm>
#include <memory>

namespace mind_map::ui {

namespace {

constexpr float kMinZoom = 0.35F;
constexpr float kMaxZoom = 3.0F;
constexpr float kZoomStep = 0.1F;

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
      if (const int sel = state.canvas_.GetSelectedChildForBranchStyle(); sel > 0) {
        // sel > 0: root (index 0) cannot be deleted
        history_.Push(std::make_unique<commands::DeleteNodeCommand>(state.canvas_, sel));
        session.MarkDirty();
      }
      return;
    }
    case UiCommandId::InsertChildNode: {
      const int sel = state.canvas_.GetSelectedChildForBranchStyle();
      const int parent = (sel > 0) ? sel : 0;  // default to root when nothing selected
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
  }
}

}  // namespace mind_map::ui
