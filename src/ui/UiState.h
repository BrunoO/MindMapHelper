#pragma once

#include "core/MindMapDocument.h"
#include "ui/MindMapCanvasView.h"

#include "imgui.h"

#include <cstdint>
#include <optional>
#include <string>

namespace mind_map::ui {

inline constexpr float kInitialPanX = 40.0F;
inline constexpr float kInitialPanY = 120.0F;

/// Destructive navigation deferred while the unsaved-changes guard modal is shown.
enum class PendingNavAction : std::uint8_t { None, New, OpenDialog, ImportDialog };

/// Which guard triggered a "Save As…" from the unsaved-changes modal; used by
/// RenderFileDialogs to resume the right action after the file dialog closes.
enum class SaveAsResumingContext : std::uint8_t { None, NavGuard, CloseGuard };

/// All per-frame mutable UI state owned by RenderMainUi: canvas view, pan/zoom, status-bar flag,
/// and any navigation action waiting behind the unsaved-changes guard.
struct UiState {
  MindMapCanvasView canvas_;
  ImVec2 pan_px_ = {kInitialPanX, kInitialPanY};
  float zoom_ = 1.0F;
  ImVec2 canvas_p0_;
  ImVec2 canvas_sz_;
  bool show_status_bar_ = true;
  PendingNavAction pending_nav_ = PendingNavAction::None;
  SaveAsResumingContext save_as_context_ = SaveAsResumingContext::None;
  // Set by RenderFileMenu/RenderFileDialogs; consumed by AppMain to launch a new process.
  // nullopt = no request; "" = blank new map; non-empty = open that file.
  std::optional<std::string> pending_launch_path_;

  [[nodiscard]] mind_map::core::MindMapViewport ToViewport() const {
    return {{pan_px_.x, pan_px_.y}, zoom_};
  }

  void ApplyViewport(const mind_map::core::MindMapViewport& vp) {
    pan_px_ = {vp.pan_.x_, vp.pan_.y_};
    zoom_ = vp.zoom_;
  }
};

}  // namespace mind_map::ui
