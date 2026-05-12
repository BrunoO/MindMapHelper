#pragma once

namespace mind_map::ui::commands {

class ICommand {
 public:
  virtual ~ICommand() = default;
  virtual void Execute() = 0;
  virtual void Undo() = 0;
};

}  // namespace mind_map::ui::commands
