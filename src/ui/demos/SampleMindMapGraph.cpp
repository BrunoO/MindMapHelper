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
  for (std::size_t i = 0; i < kSampleMindMapNodeCount; ++i) {
    node_ids[i] = mind_map::core::mindmap::GenerateUuidV4();
    if (kSampleMindMapSpecs[i].parent_idx_.has_value()) {
      edge_ids[i] = mind_map::core::mindmap::GenerateUuidV4();
    }
  }

  mind_map::core::MindMapDocument doc;

  for (std::size_t i = 0; i < kSampleMindMapNodeCount; ++i) {
    const SampleMindMapNodeSpec& spec = kSampleMindMapSpecs[i];

    mind_map::core::MindMapNode node;
    node.id_ = node_ids[i];
    node.label_ = spec.label_;
    doc.nodes_.push_back(std::move(node));

    mind_map::core::MindMapNodeLayout layout;
    layout.node_id_ = node_ids[i];
    layout.position_ = {positions[i].x, positions[i].y};
    doc.layouts_.push_back(std::move(layout));

    if (spec.parent_idx_.has_value()) {
      mind_map::core::MindMapEdge edge;
      edge.id_ = edge_ids[i];
      edge.parent_id_ = node_ids[*spec.parent_idx_];
      edge.child_id_ = node_ids[i];
      edge.style_ = std::string(kDefaultEdgeStyles[i]);
      if (i == 1U) { edge.label_ = "supports"; }
      doc.edges_.push_back(std::move(edge));
    }
  }

  doc.viewport_.pan_ = {kSampleInitialPanX, kSampleInitialPanY};
  doc.viewport_.zoom_ = 1.0F;

  return doc;
}

}  // namespace mind_map::demos
