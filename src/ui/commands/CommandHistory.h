#pragma once

#include "ui/commands/ICommand.h"

#include <memory>
#include <vector>

namespace mind_map::ui::commands {

class CommandHistory {
 public:
  // Execute cmd immediately and push it onto the undo stack; clears redo stack.
  void Push(std::unique_ptr<ICommand> cmd) {
    cmd->Execute();
    undo_stack_.push_back(std::move(cmd));
    redo_stack_.clear();
  }

  void Undo() {
    if (undo_stack_.empty()) {
      return;
    }
    auto cmd = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    cmd->Undo();
    redo_stack_.push_back(std::move(cmd));
  }

  void Redo() {
    if (redo_stack_.empty()) {
      return;
    }
    auto cmd = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    cmd->Execute();
    undo_stack_.push_back(std::move(cmd));
  }

  [[nodiscard]] bool CanUndo() const { return !undo_stack_.empty(); }
  [[nodiscard]] bool CanRedo() const { return !redo_stack_.empty(); }

 private:
  std::vector<std::unique_ptr<ICommand>> undo_stack_;
  std::vector<std::unique_ptr<ICommand>> redo_stack_;
};

}  // namespace mind_map::ui::commands
