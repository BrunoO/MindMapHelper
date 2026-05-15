#pragma once

#include "ui/commands/ICommand.h"

#include <cstddef>
#include <string>

namespace mind_map::ui {
class MindMapCanvasView;
}

namespace mind_map::ui::commands {

class UpdateEdgeLabelCommand final : public ICommand {
 public:
  UpdateEdgeLabelCommand(MindMapCanvasView& canvas, size_t node_idx,
                         std::string old_label, std::string new_label);
  void Execute() override;
  void Undo() override;

 private:
  MindMapCanvasView& canvas_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members) - safe: history.Clear() always precedes canvas replacement (see CommandHistory invariant)
  size_t node_idx_;
  std::string old_label_;
  std::string new_label_;
};

}  // namespace mind_map::ui::commands
