#pragma once

#include "ui/commands/ICommand.h"
#include "ui/MindMapCanvasView.h"

namespace mind_map::ui::commands {

class InsertChildNodeCommand final : public ICommand {
 public:
  InsertChildNodeCommand(mind_map::ui::MindMapCanvasView& canvas, int parent_idx);

  void Execute() override;
  void Undo() override;

 private:
  mind_map::ui::MindMapCanvasView& canvas_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  int parent_idx_;
  int inserted_idx_ = -1;
};

}  // namespace mind_map::ui::commands
