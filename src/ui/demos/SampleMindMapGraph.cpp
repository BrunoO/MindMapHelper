#include "ui/demos/SampleMindMapGraph.h"

#include "core/MindMapDocument.h"
#include "core/mindmap/UuidGenerator.h"

#include <array>
#include <string_view>

namespace mind_map::demos {

namespace {

// Must stay in sync with InitDefaultPerChildBranchStyles_ until that function is removed.
constexpr std::array<std::string_view, kSampleMindMapNodeCount> kDefaultEdgeStyles = {{
    {},              // 0 — root, no incoming edge
    "bezier",        // 1
    "orthogonal",    // 2
    "organic_taper", // 3
    "orthogonal",    // 4
    "bezier",        // 5
    "organic_taper", // 6
}};

constexpr float kSampleInitialPanX = 40.0F;
constexpr float kSampleInitialPanY = 120.0F;

}  // namespace

mind_map::core::MindMapDocument BuildSampleDocument() {
  const auto positions = InitialSampleMapPositions();

  std::array<std::string, kSampleMindMapNodeCount> node_ids;
  std::array<std::string, kSampleMindMapNodeCount> edge_ids;
  for (int i = 0; i < kSampleMindMapNodeCount; ++i) {
    const auto idx = static_cast<size_t>(i);
    node_ids[idx] = mind_map::core::mindmap::GenerateUuidV4();
    if (kSampleMindMapSpecs[idx].parent_ >= 0) {
      edge_ids[idx] = mind_map::core::mindmap::GenerateUuidV4();
    }
  }

  mind_map::core::MindMapDocument doc;

  for (int i = 0; i < kSampleMindMapNodeCount; ++i) {
    const auto idx = static_cast<size_t>(i);

    mind_map::core::MindMapNode node;
    node.id_ = node_ids[idx];
    node.label_ = kSampleMindMapSpecs[idx].label_;
    doc.nodes_.push_back(std::move(node));

    mind_map::core::MindMapNodeLayout layout;
    layout.node_id_ = node_ids[idx];
    layout.position_ = {positions[idx].x, positions[idx].y};
    doc.layouts_.push_back(std::move(layout));

    if (kSampleMindMapSpecs[idx].parent_ >= 0) {
      mind_map::core::MindMapEdge edge;
      edge.id_ = edge_ids[idx];
      edge.parent_id_ = node_ids[static_cast<size_t>(kSampleMindMapSpecs[idx].parent_)];
      edge.child_id_ = node_ids[idx];
      edge.style_ = std::string(kDefaultEdgeStyles[idx]);
      doc.edges_.push_back(std::move(edge));
    }
  }

  doc.viewport_.pan_ = {kSampleInitialPanX, kSampleInitialPanY};
  doc.viewport_.zoom_ = 1.0F;

  return doc;
}

}  // namespace mind_map::demos
