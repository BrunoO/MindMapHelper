#include "ui/commands/CollapseNodeCommand.h"

namespace mind_map::ui::commands {

CollapseNodeCommand::CollapseNodeCommand(mind_map::ui::MindMapCanvasView& canvas,
                                         size_t node_idx, bool collapsing)
    : canvas_(canvas), node_idx_(node_idx), collapsing_(collapsing) {}

void CollapseNodeCommand::Execute() {
  if (collapsing_) {
    canvas_.CollapseNode(node_idx_);
  } else {
    canvas_.ExpandNode(node_idx_);
  }
}

void CollapseNodeCommand::Undo() {
  if (collapsing_) {
    canvas_.ExpandNode(node_idx_);
  } else {
    canvas_.CollapseNode(node_idx_);
  }
}

}  // namespace mind_map::ui::commands
