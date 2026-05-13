#include "ui/commands/InsertChildNodeCommand.h"

#include <cassert>

namespace mind_map::ui::commands {

InsertChildNodeCommand::InsertChildNodeCommand(mind_map::ui::MindMapCanvasView& canvas, size_t parent_idx)
    : canvas_(canvas), parent_idx_(parent_idx) {}

void InsertChildNodeCommand::Execute() {
  if (!inserted_idx_.has_value()) {
    inserted_idx_ = canvas_.InsertChildNode(parent_idx_);
  } else {
    canvas_.SetNodeActive(*inserted_idx_, true);
  }
}

void InsertChildNodeCommand::Undo() {
  assert(inserted_idx_);
  canvas_.SetNodeActive(*inserted_idx_, false);
}

}  // namespace mind_map::ui::commands
