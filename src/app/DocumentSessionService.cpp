#include "app/DocumentSessionService.h"

#include "core/mindmap/UuidGenerator.h"

namespace mind_map::app {

DocumentSessionService::DocumentSessionService(mind_map::core::IDocumentRepository& repo)
    : repo_(&repo) {}

void DocumentSessionService::New(mind_map::core::MindMapDocument& doc_out) {
  doc_out = {};

  const std::string root_id = mind_map::core::mindmap::GenerateUuidV4();

  mind_map::core::MindMapNode root;
  root.id_ = root_id;
  root.label_ = "New Mind Map";
  doc_out.nodes_.push_back(std::move(root));

  mind_map::core::MindMapNodeLayout layout;
  layout.node_id_ = root_id;
  layout.position_ = {};
  doc_out.layouts_.push_back(std::move(layout));

  current_path_.clear();
  dirty_ = false;
  close_confirmed_ = false;
}

bool DocumentSessionService::Open(std::string_view path,
                                  mind_map::core::MindMapDocument& doc_out) {
  auto result = repo_->Load(path);
  if (!result.has_value()) {
    return false;
  }
  doc_out = std::move(*result);
  current_path_ = std::string{path};
  dirty_ = false;
  close_confirmed_ = false;
  return true;
}

bool DocumentSessionService::Save(const mind_map::core::MindMapDocument& doc) {
  if (current_path_.empty()) {
    return false;
  }
  if (!repo_->Save(current_path_, doc)) {
    return false;
  }
  dirty_ = false;
  return true;
}

bool DocumentSessionService::SaveAs(std::string_view path,
                                    const mind_map::core::MindMapDocument& doc) {
  if (!repo_->Save(path, doc)) {
    return false;
  }
  current_path_ = std::string{path};
  dirty_ = false;
  return true;
}

void DocumentSessionService::MarkDirty()  { dirty_ = true; }
void DocumentSessionService::ClearDirty() { dirty_ = false; }
bool DocumentSessionService::IsDirty()    const { return dirty_; }
bool DocumentSessionService::HasPath()    const { return !current_path_.empty(); }

std::string_view DocumentSessionService::GetCurrentPath() const { return current_path_; }

void DocumentSessionService::RequestClose()  { close_requested_ = true; }
void DocumentSessionService::CancelClose()   { close_requested_ = false; }
void DocumentSessionService::ConfirmClose()  { close_confirmed_ = true; }
bool DocumentSessionService::IsCloseRequested() const { return close_requested_; }
bool DocumentSessionService::IsCloseConfirmed() const { return close_confirmed_; }

}  // namespace mind_map::app
