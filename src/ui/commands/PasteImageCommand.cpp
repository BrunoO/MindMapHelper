#include "ui/commands/PasteImageCommand.h"

namespace mind_map::ui::commands {

PasteImageCommand::PasteImageCommand(mind_map::ui::MindMapCanvasView& canvas, size_t node_idx,
                                     std::string new_png_base64)
    : canvas_(canvas), node_idx_(node_idx), new_png_base64_(std::move(new_png_base64)) {}

void PasteImageCommand::Execute() {
  if (!executed_) {
    old_png_base64_ = canvas_.GetNodeImageBase64(node_idx_);
    executed_ = true;
  }
  canvas_.SetNodeImage(node_idx_, new_png_base64_);
}

void PasteImageCommand::Undo() {
  canvas_.SetNodeImage(node_idx_, old_png_base64_);
}

}  // namespace mind_map::ui::commands
