#pragma once

namespace mind_map::ui::commands {

/// Command pattern base used by CommandHistory; Execute applies the operation, Undo reverses it.
class ICommand {
 public:
  virtual ~ICommand() = default;
  virtual void Execute() = 0;
  virtual void Undo() = 0;
};

}  // namespace mind_map::ui::commands
