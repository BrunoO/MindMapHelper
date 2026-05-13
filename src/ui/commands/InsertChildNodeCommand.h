#pragma once

#include "ui/commands/ICommand.h"
#include "ui/MindMapCanvasView.h"

#include <cstddef>
#include <optional>

namespace mind_map::ui::commands {

class InsertChildNodeCommand final : public ICommand {
 public:
  InsertChildNodeCommand(mind_map::ui::MindMapCanvasView& canvas, size_t parent_idx);

  void Execute() override;
  void Undo() override;

 private:
  mind_map::ui::MindMapCanvasView& canvas_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
  size_t parent_idx_;
  std::optional<size_t> inserted_idx_;
};

}  // namespace mind_map::ui::commands
