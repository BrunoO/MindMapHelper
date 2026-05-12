#pragma once

#include "ui/commands/ICommand.h"
#include "ui/MindMapCanvasView.h"

#include <string>

namespace mind_map::ui::commands {

class PasteImageCommand final : public ICommand {
 public:
  PasteImageCommand(mind_map::ui::MindMapCanvasView& canvas, int node_idx,
                    std::string new_png_base64);

  void Execute() override;
  void Undo() override;

 private:
  mind_map::ui::MindMapCanvasView& canvas_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  int node_idx_;
  std::string new_png_base64_;
  std::string old_png_base64_;  // captured on first Execute for Undo
  bool executed_ = false;
};

}  // namespace mind_map::ui::commands
