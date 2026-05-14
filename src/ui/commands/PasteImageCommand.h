#pragma once

#include "ui/commands/ICommand.h"
#include "ui/MindMapCanvasView.h"

#include <string>

namespace mind_map::ui::commands {

/// ICommand that calls MindMapCanvasView::SetNodeImage with a new PNG; Undo restores the previous image.
class PasteImageCommand final : public ICommand {
 public:
  PasteImageCommand(mind_map::ui::MindMapCanvasView& canvas, size_t node_idx,
                    std::string new_png_base64);

  void Execute() override;
  void Undo() override;

 private:
  mind_map::ui::MindMapCanvasView& canvas_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  size_t node_idx_;
  std::string new_png_base64_;
  std::string old_png_base64_;  // captured on first Execute for Undo
  bool executed_ = false;
};

}  // namespace mind_map::ui::commands
