#include "ui/commands/PasteTextCommand.h"

namespace mind_map::ui::commands {

PasteTextCommand::PasteTextCommand(mind_map::ui::MindMapCanvasView& canvas, size_t node_idx,
                                   std::string new_label)
    : canvas_(canvas), node_idx_(node_idx), new_label_(std::move(new_label)) {}

void PasteTextCommand::Execute() {
  if (!executed_) {
    old_label_ = canvas_.GetNodeLabel(node_idx_);
    executed_ = true;
  }
  canvas_.SetNodeLabel(node_idx_, new_label_);
}

void PasteTextCommand::Undo() {
  canvas_.SetNodeLabel(node_idx_, old_label_);
}

}  // namespace mind_map::ui::commands
