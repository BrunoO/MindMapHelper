#include "app/DocumentSessionService.h"

#include "utils/Logger.h"

#include "core/DocumentPathLoader.h"
#include "core/mindmap/UuidGenerator.h"

namespace mind_map::app {

DocumentSessionService::DocumentSessionService(mind_map::core::IDocumentRepository& repo)
    : repo_(&repo) {}

void DocumentSessionService::ResetCloseGuardState_() {
  close_confirmed_ = false;
  close_requested_ = false;
}

void DocumentSessionService::SetNativeSessionAfterOpen_(const std::string_view path) {
  current_path_ = std::string{path};
  dirty_ = false;
  ResetCloseGuardState_();
}

void DocumentSessionService::SetImportedSessionAfterOpen_() {
  current_path_.clear();
  dirty_ = true;
  ResetCloseGuardState_();
}

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
  ResetCloseGuardState_();
}

bool DocumentSessionService::OpenNative(const std::string_view path,
                                        mind_map::core::MindMapDocument& doc_out) {
  auto result = repo_->Load(path);
  if (!result.has_value()) {
    return false;
  }
  doc_out = std::move(*result);
  SetNativeSessionAfterOpen_(path);
  LOG_INFO_BUILD("DocumentSessionService::OpenNative: opened '" << path << '\'');
  return true;
}

bool DocumentSessionService::OpenFromPath(const std::string_view path,
                                          mind_map::core::MindMapDocument& doc_out,
                                          const mind_map::core::ImportService& imports) {
  mind_map::core::DocumentPathLoadResult loaded =
      mind_map::core::LoadMindMapFromPath(path, *repo_, imports);
  if (loaded.outcome_ == mind_map::core::DocumentPathLoadOutcome::Failed) {
    return false;
  }
  doc_out = std::move(loaded.doc_);
  if (loaded.outcome_ == mind_map::core::DocumentPathLoadOutcome::NativeAtPath) {
    SetNativeSessionAfterOpen_(path);
    LOG_INFO_BUILD("DocumentSessionService::OpenFromPath: opened native '" << path << '\'');
  } else {
    SetImportedSessionAfterOpen_();
    LOG_INFO_BUILD("DocumentSessionService::OpenFromPath: imported '" << path << '\'');
  }
  return true;
}

void DocumentSessionService::ApplyImportedDocument() { SetImportedSessionAfterOpen_(); }

bool DocumentSessionService::Save(const mind_map::core::MindMapDocument& doc) {
  if (current_path_.empty()) {
    LOG_WARNING_BUILD("DocumentSessionService::Save: no current path (use Save As)");
    return false;
  }
  if (!repo_->Save(current_path_, doc)) {
    return false;
  }
  dirty_ = false;
  LOG_INFO_BUILD("DocumentSessionService::Save: saved '" << current_path_ << '\'');
  return true;
}

bool DocumentSessionService::SaveAs(const std::string_view path,
                                    const mind_map::core::MindMapDocument& doc) {
  if (!repo_->Save(path, doc)) {
    return false;
  }
  current_path_ = std::string{path};
  dirty_ = false;
  LOG_INFO_BUILD("DocumentSessionService::SaveAs: saved '" << path << '\'');
  return true;
}

void DocumentSessionService::MarkDirty() { dirty_ = true; }
void DocumentSessionService::ClearDirty() { dirty_ = false; }
bool DocumentSessionService::IsDirty() const { return dirty_; }
bool DocumentSessionService::HasPath() const { return !current_path_.empty(); }

std::string_view DocumentSessionService::GetCurrentPath() const { return current_path_; }

void DocumentSessionService::RequestClose() { close_requested_ = true; }
void DocumentSessionService::CancelClose() { close_requested_ = false; }
void DocumentSessionService::ConfirmClose() { close_confirmed_ = true; }
bool DocumentSessionService::IsCloseRequested() const { return close_requested_; }
bool DocumentSessionService::IsCloseConfirmed() const { return close_confirmed_; }

}  // namespace mind_map::app
