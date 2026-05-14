#pragma once

#include "ui/commands/ICommand.h"
#include "ui/MindMapCanvasView.h"

#include <string>

namespace mind_map::ui::commands {

/// ICommand that replaces a node's label with clipboard text; Undo restores the previous label.
class PasteTextCommand final : public ICommand {
 public:
  PasteTextCommand(mind_map::ui::MindMapCanvasView& canvas, size_t node_idx, std::string new_label);

  void Execute() override;
  void Undo() override;

 private:
  mind_map::ui::MindMapCanvasView& canvas_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  size_t node_idx_;
  std::string new_label_;
  std::string old_label_;  // captured on first Execute for Undo
  bool executed_ = false;
};

}  // namespace mind_map::ui::commands
