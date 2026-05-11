#pragma once

#include "core/IDocumentRepository.h"
#include "core/MindMapDocument.h"

#include <string>
#include <string_view>

namespace mind_map::app {

// Owns the single-document lifecycle: path, dirty flag, and close-guard state.
// All canvas I/O (LoadFrom / ToDocument) is the caller's responsibility.
class DocumentSessionService {
 public:
  explicit DocumentSessionService(mind_map::core::IDocumentRepository& repo);

  // Populates doc_out with a minimal one-root-node document; clears path and dirty.
  void New(mind_map::core::MindMapDocument& doc_out);

  // Loads from path into doc_out. Returns false and leaves doc_out unchanged on failure.
  [[nodiscard]] bool Open(std::string_view path, mind_map::core::MindMapDocument& doc_out);

  // Saves doc to current_path_. Returns false when no path is set — caller must use SaveAs.
  [[nodiscard]] bool Save(const mind_map::core::MindMapDocument& doc);

  // Saves doc to the given path and updates current_path_.
  [[nodiscard]] bool SaveAs(std::string_view path, const mind_map::core::MindMapDocument& doc);

  void MarkDirty();
  void ClearDirty();
  [[nodiscard]] bool IsDirty() const;
  [[nodiscard]] bool HasPath() const;
  [[nodiscard]] std::string_view GetCurrentPath() const;

  // Close guard: RequestClose() is called by AppMain when the window is closing dirty.
  // RenderMainUi checks IsCloseRequested() each frame and shows a confirmation modal.
  // The modal calls ConfirmClose() (proceed) or CancelClose() (abort).
  // AppMain checks IsCloseConfirmed() to break the render loop.
  void RequestClose();
  void CancelClose();
  void ConfirmClose();
  [[nodiscard]] bool IsCloseRequested() const;
  [[nodiscard]] bool IsCloseConfirmed() const;

 private:
  mind_map::core::IDocumentRepository* repo_;
  std::string current_path_;
  bool dirty_ = false;
  bool close_requested_ = false;
  bool close_confirmed_ = false;
};

}  // namespace mind_map::app
