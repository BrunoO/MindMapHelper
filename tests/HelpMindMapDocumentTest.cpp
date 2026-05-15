#include "app/HelpMindMapDocument.h"
#include "core/MindMapDocument.h"

#include <cassert>
#include <cstddef>

namespace {

void AssertEachNonRootHasExactlyOneParent(const mind_map::core::MindMapDocument& doc) {
  assert(!doc.nodes_.empty());
  const std::string& root_id = doc.nodes_[0].id_;
  for (const auto& node : doc.nodes_) {
    if (node.id_ == root_id) {
      continue;
    }
    int parent_edges = 0;
    for (const auto& edge : doc.edges_) {
      if (edge.child_id_ == node.id_) {
        ++parent_edges;
      }
    }
    assert(parent_edges == 1);
  }
}

}  // namespace

int main() {
  constexpr float kExpectedViewportPanX = 40.0F;
  constexpr float kExpectedViewportPanY = 120.0F;
  constexpr float kExpectedViewportZoom   = 1.0F;

  constexpr std::size_t kMinHelpNodeCount = 20U;

  const mind_map::core::MindMapDocument doc = mind_map::app::BuildHelpMindMapDocument();
  assert(doc.nodes_.size() >= kMinHelpNodeCount);
  assert(doc.edges_.size() + 1U == doc.nodes_.size());
  assert(doc.nodes_[0].label_.find("guide") != std::string::npos);
  bool has_log_help = false;
  for (const auto& node : doc.nodes_) {
    if (node.label_.find("Session log") != std::string::npos) {
      has_log_help = true;
      break;
    }
  }
  assert(has_log_help);
  assert(doc.layouts_.size() == doc.nodes_.size());
  assert(doc.viewport_.pan_.x_ == kExpectedViewportPanX && doc.viewport_.pan_.y_ == kExpectedViewportPanY);
  assert(doc.viewport_.zoom_ == kExpectedViewportZoom);
  AssertEachNonRootHasExactlyOneParent(doc);
  return 0;
}
