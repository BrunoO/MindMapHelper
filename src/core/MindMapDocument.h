#pragma once

#include <string>
#include <vector>

namespace mind_map::core {

struct Vec2 {
  float x_ = 0.0F;
  float y_ = 0.0F;
};

/// A node in the mind map: unique id, display label, and optional embedded image (base64 PNG).
struct MindMapNode {
  std::string id_;
  std::string label_;
  std::string image_png_base64_;  // base64-encoded PNG bytes; empty when no image
  bool collapsed_ = false;        // children hidden; persisted so the fold state survives save/load
};

/// A directed parent→child edge with optional style override and display label.
struct MindMapEdge {
  std::string id_;
  std::string parent_id_;
  std::string child_id_;
  std::string style_;  // "bezier" | "orthogonal" | "organic_taper"
  std::string label_;  // caption for the edge from parent_id_ to child_id_; empty = no edge label
};

/// Persisted position and size hint for a node; size {0,0} means auto-fit to label.
struct MindMapNodeLayout {
  std::string node_id_;
  Vec2 position_;
  float size_w_ = 0.0F;  // half-width override; 0 = auto-size from label
  float size_h_ = 0.0F;  // half-height override; 0 = auto-size from label
};

/// Saved camera state (pan, zoom) restored when the document is reopened.
struct MindMapViewport {
  Vec2 pan_;
  float zoom_ = 1.0F;
};

/// Full serializable mind-map state: nodes, edges, layout hints, and viewport; exchanged between core and ui layers.
struct MindMapDocument {
  std::vector<MindMapNode> nodes_;
  std::vector<MindMapEdge> edges_;
  std::vector<MindMapNodeLayout> layouts_;
  MindMapViewport viewport_;
};

}  // namespace mind_map::core
