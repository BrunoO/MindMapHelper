#include "ui/commands/CommandHistory.h"
#include "ui/commands/ICommand.h"

#include <cassert>
#include <memory>

namespace {

// Records how many times Execute and Undo have been called.
class CountingCommand final : public mind_map::ui::commands::ICommand {
 public:
  int execute_count_ = 0;
  int undo_count_ = 0;

  void Execute() override { ++execute_count_; }
  void Undo() override { ++undo_count_; }
};

void TestPushExecutesImmediately() {
  mind_map::ui::commands::CommandHistory history;
  auto cmd = std::make_unique<CountingCommand>();
  const CountingCommand* raw = cmd.get();
  history.Push(std::move(cmd));
  assert(raw->execute_count_ == 1);
  assert(raw->undo_count_ == 0);
  assert(history.CanUndo());
  assert(!history.CanRedo());
}

void TestUndoAndRedo() {
  mind_map::ui::commands::CommandHistory history;
  auto cmd = std::make_unique<CountingCommand>();
  const CountingCommand* raw = cmd.get();
  history.Push(std::move(cmd));

  history.Undo();
  assert(raw->undo_count_ == 1);
  assert(!history.CanUndo());
  assert(history.CanRedo());

  history.Redo();
  assert(raw->execute_count_ == 2);
  assert(history.CanUndo());
  assert(!history.CanRedo());
}

void TestChainedUndoRedo() {
  mind_map::ui::commands::CommandHistory history;

  auto a = std::make_unique<CountingCommand>();
  auto b = std::make_unique<CountingCommand>();
  const CountingCommand* raw_a = a.get();
  const CountingCommand* raw_b = b.get();
  history.Push(std::move(a));
  history.Push(std::move(b));

  assert(raw_a->execute_count_ == 1);
  assert(raw_b->execute_count_ == 1);

  history.Undo();  // undo b
  assert(raw_b->undo_count_ == 1);
  assert(history.CanUndo());
  assert(history.CanRedo());

  history.Undo();  // undo a
  assert(raw_a->undo_count_ == 1);
  assert(!history.CanUndo());
  assert(history.CanRedo());

  history.Redo();  // redo a
  assert(raw_a->execute_count_ == 2);
  history.Redo();  // redo b
  assert(raw_b->execute_count_ == 2);
  assert(history.CanUndo());
  assert(!history.CanRedo());
}

void TestNewCommandClearsRedoStack() {
  mind_map::ui::commands::CommandHistory history;
  auto a = std::make_unique<CountingCommand>();
  auto b = std::make_unique<CountingCommand>();
  auto c = std::make_unique<CountingCommand>();

  history.Push(std::move(a));
  history.Push(std::move(b));
  history.Undo();
  assert(history.CanRedo());

  history.Push(std::move(c));
  assert(!history.CanRedo());
  assert(history.CanUndo());
}

void TestUndoOnEmptyIsNoop() {
  mind_map::ui::commands::CommandHistory history;
  history.Undo();  // must not crash
  assert(!history.CanUndo());
  assert(!history.CanRedo());
}

void TestRedoOnEmptyIsNoop() {
  mind_map::ui::commands::CommandHistory history;
  history.Redo();  // must not crash
  assert(!history.CanRedo());
}

}  // namespace

// NOLINTNEXTLINE(bugprone-exception-escape) - test driver; failures use assert
int main() {
  TestPushExecutesImmediately();
  TestUndoAndRedo();
  TestChainedUndoRedo();
  TestNewCommandClearsRedoStack();
  TestUndoOnEmptyIsNoop();
  TestRedoOnEmptyIsNoop();
  return 0;
}
