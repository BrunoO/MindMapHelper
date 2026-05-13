#pragma once

#include "ui/commands/ICommand.h"
#include "ui/MindMapCanvasView.h"

#include <cstddef>
#include <vector>

namespace mind_map::ui::commands {

class DeleteNodeCommand final : public ICommand {
 public:
  DeleteNodeCommand(mind_map::ui::MindMapCanvasView& canvas, size_t node_idx);

  void Execute() override;
  void Undo() override;

 private:
  mind_map::ui::MindMapCanvasView& canvas_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members) - bound to canvas lifetime for undo
  size_t node_idx_;
  std::vector<size_t> affected_indices_;
};

}  // namespace mind_map::ui::commands
