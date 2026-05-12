#include "ui/commands/InsertChildNodeCommand.h"

namespace mind_map::ui::commands {

InsertChildNodeCommand::InsertChildNodeCommand(mind_map::ui::MindMapCanvasView& canvas, int parent_idx)
    : canvas_(canvas), parent_idx_(parent_idx) {}

void InsertChildNodeCommand::Execute() {
  if (inserted_idx_ < 0) {
    inserted_idx_ = canvas_.InsertChildNode(parent_idx_);
  } else {
    canvas_.SetNodeActive(inserted_idx_, true);
  }
}

void InsertChildNodeCommand::Undo() {
  canvas_.SetNodeActive(inserted_idx_, false);
}

}  // namespace mind_map::ui::commands
