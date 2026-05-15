#include "ui/commands/RenameNodeCommand.h"

#include "ui/MindMapCanvasView.h"
#include "ui/canvas/MindMapCanvasNodeMutators.h"

namespace mind_map::ui::commands {

RenameNodeCommand::RenameNodeCommand(MindMapCanvasView& canvas, size_t node_idx,
                                     std::string old_label, std::string new_label)
    : canvas_(canvas),
      node_idx_(node_idx),
      old_label_(std::move(old_label)),
      new_label_(std::move(new_label)) {}

void RenameNodeCommand::Execute() {
  canvas::SetNodeLabel(canvas_, node_idx_, new_label_);
}

void RenameNodeCommand::Undo() {
  canvas::SetNodeLabel(canvas_, node_idx_, old_label_);
}

}  // namespace mind_map::ui::commands
