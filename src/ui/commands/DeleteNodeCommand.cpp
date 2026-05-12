#include "ui/commands/DeleteNodeCommand.h"

namespace mind_map::ui::commands {

DeleteNodeCommand::DeleteNodeCommand(mind_map::ui::MindMapCanvasView& canvas, int node_idx)
    : canvas_(canvas), node_idx_(node_idx) {}

void DeleteNodeCommand::Execute() {
  affected_indices_ = canvas_.CollectActiveSubtree(node_idx_);
  for (const int idx : affected_indices_) {
    canvas_.SetNodeActive(idx, false);
  }
}

void DeleteNodeCommand::Undo() {
  for (const int idx : affected_indices_) {
    canvas_.SetNodeActive(idx, true);
  }
}

}  // namespace mind_map::ui::commands
