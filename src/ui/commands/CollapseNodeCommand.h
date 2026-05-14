#pragma once

#include "ui/commands/ICommand.h"
#include "ui/MindMapCanvasView.h"

#include <cstddef>

namespace mind_map::ui::commands {

/// ICommand that collapses or expands a subtree; Execute/Undo are symmetric inverses.
class CollapseNodeCommand final : public ICommand {
 public:
  CollapseNodeCommand(mind_map::ui::MindMapCanvasView& canvas, size_t node_idx, bool collapsing);

  void Execute() override;
  void Undo() override;

 private:
  mind_map::ui::MindMapCanvasView& canvas_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  size_t node_idx_;
  bool collapsing_;
};

}  // namespace mind_map::ui::commands
