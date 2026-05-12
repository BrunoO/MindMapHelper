#pragma once

#include "ui/commands/ICommand.h"
#include "ui/MindMapCanvasView.h"

#include <vector>

namespace mind_map::ui::commands {

class DeleteNodeCommand final : public ICommand {
 public:
  DeleteNodeCommand(mind_map::ui::MindMapCanvasView& canvas, int node_idx);

  void Execute() override;
  void Undo() override;

 private:
  mind_map::ui::MindMapCanvasView& canvas_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members) - bound to canvas lifetime for undo
  int node_idx_;
  std::vector<int> affected_indices_;
};

}  // namespace mind_map::ui::commands
