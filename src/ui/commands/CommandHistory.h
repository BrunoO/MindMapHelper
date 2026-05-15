#pragma once

#include "ui/commands/ICommand.h"

#include <memory>
#include <vector>

namespace mind_map::ui::commands {

/// Undo/redo stack for ICommand objects; Push executes immediately and clears the redo stack.
///
/// Lifetime invariant: commands may store raw references to canvas/session objects.
/// Clear() MUST be called before the referenced objects are destroyed or replaced
/// (e.g., before LoadFrom on a new document). All document-load paths in MindMapUi.cpp
/// already do this. If in-process multi-canvas support is ever added, audit every
/// canvas-replacing call site to ensure Clear() precedes it.
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

  void Clear() {
    undo_stack_.clear();
    redo_stack_.clear();
  }

 private:
  std::vector<std::unique_ptr<ICommand>> undo_stack_;
  std::vector<std::unique_ptr<ICommand>> redo_stack_;
};

}  // namespace mind_map::ui::commands
