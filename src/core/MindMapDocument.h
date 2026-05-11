#pragma once

#include <string>
#include <vector>

namespace mind_map::core {

struct Vec2 {
  float x_ = 0.0F;
  float y_ = 0.0F;
};

struct MindMapNode {
  std::string id_;
  std::string label_;
};

struct MindMapEdge {
  std::string id_;
  std::string parent_id_;
  std::string child_id_;
  std::string style_;  // "bezier" | "orthogonal" | "organic_taper"
};

struct MindMapNodeLayout {
  std::string node_id_;
  Vec2 position_;
};

struct MindMapViewport {
  Vec2 pan_;
  float zoom_ = 1.0F;
};

struct MindMapDocument {
  std::vector<MindMapNode> nodes_;
  std::vector<MindMapEdge> edges_;
  std::vector<MindMapNodeLayout> layouts_;
  MindMapViewport viewport_;
};

}  // namespace mind_map::core
