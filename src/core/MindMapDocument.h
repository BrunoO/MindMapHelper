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
  std::string image_png_base64_;  // base64-encoded PNG bytes; empty when no image
};

struct MindMapEdge {
  std::string id_;
  std::string parent_id_;
  std::string child_id_;
  std::string style_;  // "bezier" | "orthogonal" | "organic_taper"
  std::string label_;  // caption for the edge from parent_id_ to child_id_; empty = no edge label
};

struct MindMapNodeLayout {
  std::string node_id_;
  Vec2 position_;
  float size_w_ = 0.0F;  // half-width override; 0 = auto-size from label
  float size_h_ = 0.0F;  // half-height override; 0 = auto-size from label
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
