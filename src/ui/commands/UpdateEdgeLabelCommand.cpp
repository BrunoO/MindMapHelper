#include "ui/commands/UpdateEdgeLabelCommand.h"

#include "ui/MindMapCanvasView.h"
#include "ui/canvas/MindMapCanvasNodeMutators.h"

namespace mind_map::ui::commands {

UpdateEdgeLabelCommand::UpdateEdgeLabelCommand(MindMapCanvasView& canvas, size_t node_idx,
                                               std::string old_label, std::string new_label)
    : canvas_(canvas),
      node_idx_(node_idx),
      old_label_(std::move(old_label)),
      new_label_(std::move(new_label)) {}

void UpdateEdgeLabelCommand::Execute() {
  canvas::SetEdgeLabel(canvas_, node_idx_, new_label_);
}

void UpdateEdgeLabelCommand::Undo() {
  canvas::SetEdgeLabel(canvas_, node_idx_, old_label_);
}

}  // namespace mind_map::ui::commands
