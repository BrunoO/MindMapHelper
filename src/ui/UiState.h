#pragma once

#include "core/MindMapDocument.h"
#include "ui/MindMapCanvasView.h"

#include "imgui.h"

#include <cstdint>

namespace mind_map::ui {

inline constexpr float kInitialPanX = 40.0F;
inline constexpr float kInitialPanY = 120.0F;

/// Destructive navigation deferred while the unsaved-changes guard modal is shown.
enum class PendingNavAction : std::uint8_t { None, New, OpenDialog, ImportDialog };

/// All per-frame mutable UI state owned by RenderMainUi: canvas view, pan/zoom, status-bar flag,
/// and any navigation action waiting behind the unsaved-changes guard.
struct UiState {
  MindMapCanvasView canvas_;
  ImVec2 pan_px_ = {kInitialPanX, kInitialPanY};
  float zoom_ = 1.0F;
  bool show_status_bar_ = true;
  PendingNavAction pending_nav_ = PendingNavAction::None;

  [[nodiscard]] mind_map::core::MindMapViewport ToViewport() const {
    return {{pan_px_.x, pan_px_.y}, zoom_};
  }

  void ApplyViewport(const mind_map::core::MindMapViewport& vp) {
    pan_px_ = {vp.pan_.x_, vp.pan_.y_};
    zoom_ = vp.zoom_;
  }
};

}  // namespace mind_map::ui
