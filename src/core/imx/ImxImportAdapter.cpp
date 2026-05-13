#include "core/imx/ImxImportAdapter.h"

#include "core/Base64.h"
#include "core/imx/ImxMindMapLoader.h"
#include "core/imx/ImxMindMapModel.h"
#include "core/mindmap/UuidGenerator.h"

#include <algorithm>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace mind_map::core {

namespace {

constexpr float kXSpacing = 260.0F;
constexpr float kYSpacing = 120.0F;
constexpr const char* kDefaultEdgeStyle = "bezier";

// Maps every IMX node id to a freshly generated UUID v4.
[[nodiscard]] std::unordered_map<std::string, std::string> BuildIdToUuidMap(
    const ImxMindMapModel& model) {
  std::unordered_map<std::string, std::string> map;
  map.reserve(model.nodes_.size());
  for (const ImxNode& node : model.nodes_) {
    map.try_emplace(node.id_, mind_map::core::mindmap::GenerateUuidV4());
  }
  return map;
}

[[nodiscard]] std::vector<MindMapNode> BuildDocumentNodes(
    const ImxMindMapModel& model,
    const std::unordered_map<std::string, std::string>& id_to_uuid) {
  std::vector<MindMapNode> nodes;
  nodes.reserve(model.nodes_.size());
  for (const ImxNode& imx_node : model.nodes_) {
    MindMapNode node;
    node.id_ = id_to_uuid.at(imx_node.id_);
    node.label_ = imx_node.text_;
    if (!imx_node.image_bytes_.empty()) {
      node.image_png_base64_ = Base64Encode(imx_node.image_bytes_);
    }
    nodes.push_back(std::move(node));
  }
  return nodes;
}

[[nodiscard]] std::vector<MindMapEdge> BuildDocumentEdges(
    const ImxMindMapModel& model,
    const std::unordered_map<std::string, std::string>& id_to_uuid) {
  std::unordered_map<std::string, std::string> child_branch_text;
  child_branch_text.reserve(model.nodes_.size());
  for (const ImxNode& n : model.nodes_) {
    if (!n.incoming_branch_text_.empty()) {
      child_branch_text.try_emplace(n.id_, n.incoming_branch_text_);
    }
  }

  std::vector<MindMapEdge> edges;
  for (const ImxNode& imx_node : model.nodes_) {
    for (const std::string& child_id : imx_node.children_) {
      const auto child_it = id_to_uuid.find(child_id);
      if (child_it == id_to_uuid.end()) {
        continue;  // dangling branch reference — skip
      }
      MindMapEdge edge;
      edge.id_ = mind_map::core::mindmap::GenerateUuidV4();
      edge.parent_id_ = id_to_uuid.at(imx_node.id_);
      edge.child_id_ = child_it->second;
      edge.style_ = kDefaultEdgeStyle;
      if (const auto bt = child_branch_text.find(child_id); bt != child_branch_text.end()) {
        edge.label_ = bt->second;
      }
      edges.push_back(std::move(edge));
    }
  }
  return edges;
}

// BFS from root; returns depth per IMX id (unreachable nodes get depth 0).
[[nodiscard]] std::unordered_map<std::string, int> BuildDepthMap(const ImxMindMapModel& model) {
  std::unordered_map<std::string, int> depth;
  depth.reserve(model.nodes_.size());

  // index nodes by id for O(1) child lookup
  std::unordered_map<std::string, const ImxNode*> by_id;
  by_id.reserve(model.nodes_.size());
  for (const ImxNode& n : model.nodes_) {
    by_id.try_emplace(n.id_, &n);
  }

  std::deque<std::string> queue;
  depth[model.root_id_] = 0;
  queue.push_back(model.root_id_);

  while (!queue.empty()) {
    const std::string curr = std::move(queue.front());
    queue.pop_front();
    const auto node_it = by_id.find(curr);
    if (node_it == by_id.end()) {
      continue;
    }
    const int child_depth = depth.at(curr) + 1;
    for (const std::string& child_id : node_it->second->children_) {
      if (depth.find(child_id) == depth.end()) {
        depth[child_id] = child_depth;
        queue.push_back(child_id);
      }
    }
  }
  return depth;
}

// Groups IMX ids by depth (preserving BFS order within each level).
[[nodiscard]] std::unordered_map<int, std::vector<std::string>> GroupByDepth(
    const ImxMindMapModel& model,
    const std::unordered_map<std::string, int>& depth_map) {
  std::unordered_map<int, std::vector<std::string>> groups;
  for (const ImxNode& node : model.nodes_) {
    const auto it = depth_map.find(node.id_);
    const int depth = it != depth_map.end() ? it->second : 0;
    groups[depth].push_back(node.id_);
  }
  return groups;
}

[[nodiscard]] std::vector<MindMapNodeLayout> BuildDocumentLayouts(
    const ImxMindMapModel& model,
    const std::unordered_map<std::string, std::string>& id_to_uuid) {
  const std::unordered_map<std::string, int> depth_map = BuildDepthMap(model);
  const std::unordered_map<int, std::vector<std::string>> groups = GroupByDepth(model, depth_map);

  std::vector<MindMapNodeLayout> layouts;
  layouts.reserve(model.nodes_.size());

  for (const auto& [depth, ids] : groups) {
    const auto count = static_cast<float>(ids.size());
    for (size_t i = 0; i < ids.size(); ++i) {
      const auto uuid_it = id_to_uuid.find(ids[i]);
      if (uuid_it == id_to_uuid.end()) {
        continue;
      }
      MindMapNodeLayout layout;
      layout.node_id_ = uuid_it->second;
      layout.position_.x_ = static_cast<float>(depth) * kXSpacing;
      layout.position_.y_ = (static_cast<float>(i) - (count - 1.0F) * 0.5F) * kYSpacing;
      layouts.push_back(std::move(layout));
    }
  }
  return layouts;
}

[[nodiscard]] MindMapDocument ImxModelToDocument(const ImxMindMapModel& model) {
  const auto id_to_uuid = BuildIdToUuidMap(model);

  MindMapDocument doc;
  doc.nodes_   = BuildDocumentNodes(model, id_to_uuid);
  doc.edges_   = BuildDocumentEdges(model, id_to_uuid);
  doc.layouts_ = BuildDocumentLayouts(model, id_to_uuid);
  return doc;
}

}  // namespace

std::optional<MindMapDocument> ImxImportAdapter::Import(std::string_view path) const {
  const std::optional<ImxMindMapModel> model = LoadImxMindMapModelFromFile(path);
  if (!model) {
    return std::nullopt;
  }
  return ImxModelToDocument(*model);
}

std::optional<MindMapDocument> ImxImportAdapter::ImportFromXml(std::string_view data_xml,
                                                                std::string_view mapmeta_xml) const {
  const std::optional<ImxMindMapModel> model = LoadImxMindMapModelFromXml(data_xml, mapmeta_xml);
  if (!model) {
    return std::nullopt;
  }
  return ImxModelToDocument(*model);
}

}  // namespace mind_map::core
